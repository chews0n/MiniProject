import sys
import subprocess
import csv
import matplotlib.pyplot as plt
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QLineEdit, QPushButton, QVBoxLayout, QWidget, QFileDialog, QMessageBox, QCheckBox
from PyQt5.QtCore import Qt
import os

class PressureAnalysisUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Pressure Analysis")
        self.setGeometry(100, 100, 400, 300)
        self.central_widget = QWidget(self)
        self.setCentralWidget(self.central_widget)
        self.layout = QVBoxLayout(self.central_widget)

        self.input_file_label = QLabel("Input File:")
        self.input_file_edit = QLineEdit()
        self.input_file_button = QPushButton("Browse")
        self.input_file_button.clicked.connect(self.browse_input_file)

        self.well_name_label = QLabel("Well Name:")
        self.well_name_edit = QLineEdit()

        self.auto_slope_check = QCheckBox("Auto Slope")
        self.slope_label = QLabel("Slope:")
        self.slope_edit = QLineEdit()
        self.slope_edit.setEnabled(False)
        self.auto_slope_check.stateChanged.connect(self.on_auto_slope_changed)

        self.test_start_label = QLabel("Test Start Time (Hours):")
        self.test_start_edit = QLineEdit()

        self.test_end_label = QLabel("Test End Time (Hours):")
        self.test_end_edit = QLineEdit()

        self.analyze_button = QPushButton("Analyze")
        self.analyze_button.clicked.connect(self.analyze_pressure)

        self.layout.addWidget(self.input_file_label)
        self.layout.addWidget(self.input_file_edit)
        self.layout.addWidget(self.input_file_button)
        self.layout.addWidget(self.well_name_label)
        self.layout.addWidget(self.well_name_edit)
        self.layout.addWidget(self.auto_slope_check)
        self.layout.addWidget(self.slope_label)
        self.layout.addWidget(self.slope_edit)
        self.layout.addWidget(self.test_start_label)
        self.layout.addWidget(self.test_start_edit)
        self.layout.addWidget(self.test_end_label)
        self.layout.addWidget(self.test_end_edit)
        self.layout.addWidget(self.analyze_button)

    def browse_input_file(self):
        file_path, _ = QFileDialog.getOpenFileName(self, "Select Input File", "", "CSV Files (*.csv);;Excel Files (*.xlsx)")
        self.input_file_edit.setText(file_path)

    def on_auto_slope_changed(self, state):
        if state == Qt.Checked:
            self.slope_edit.setEnabled(False)
        else:
            self.slope_edit.setEnabled(True)

    def analyze_pressure(self):
        input_file = self.input_file_edit.text()
        well_name = self.well_name_edit.text()
        slope = self.slope_edit.text()
        test_start = self.test_start_edit.text()
        test_end = self.test_end_edit.text()

        if not input_file or not well_name or not test_start or not test_end:
            QMessageBox.warning(self, "Missing Input", "Please provide all the required inputs.")
            return

        slope = "auto" if self.auto_slope_check.isChecked() else slope

        try:
            result = subprocess.run(["./backend", input_file, well_name, slope, test_start, test_end], capture_output=True, text=True)
            if result.returncode != 0:
                QMessageBox.critical(self, "Error", f"An error occurred:\n{result.stderr}")
                return
            print("Backend output:")
            print(result.stdout)
            self.plot_graph(well_name)
        except FileNotFoundError:
            QMessageBox.critical(self, "Error", "Backend executable not found.")

    def plot_graph(self, well_name):
        try:
            plot_file_name = f"{well_name}_test_2_plot_data.csv"

            if not os.path.exists(plot_file_name):
                raise FileNotFoundError("Plot data file not found.")

            data = self.read_csv(plot_file_name)
            times = [float(row[0]) for row in data if row[0]]
            observed_bhp = [float(row[1]) for row in data if row[1]]
            predicted_pressures = [float(row[2]) for row in data if row[2]]

            if len(times) > 1:
                slope = (predicted_pressures[-1] - predicted_pressures[0]) / (times[-1] - times[0])
            else:
                slope = 0

            plt.figure(figsize=(12, 8))
            observed_line, = plt.plot(times, observed_bhp, 'b-', label='Observed BHP')
            predicted_line, = plt.plot(times, predicted_pressures, 'r--', label='Predicted Pressures')

            test_start_time = float(self.test_start_edit.text())
            test_end_time = float(self.test_end_edit.text())

            plt.axvline(x=test_start_time, color='g', linestyle='--', label='Test Start Time')
            plt.axvline(x=test_end_time, color='y', linestyle='--', label='Test End Time')

            plt.title(f'{well_name} Pressure vs Time')
            plt.xlabel('Time (Hours)')
            plt.ylabel('Pressure (psi)')
            plt.grid(True)

            # Legend
            plt.legend(handles=[observed_line, predicted_line, 
                                plt.Line2D([], [], color='g', linestyle='--', label=f'Test Start: {test_start_time} hrs'),
                                plt.Line2D([], [], color='y', linestyle='--', label=f'Test End: {test_end_time} hrs'),
                                plt.Line2D([], [], color='none', label=f'Slope: {slope:.2f} psi/hr')],
                    loc='best')

            plt.tight_layout()

            graph_filename = f"{well_name}_pressure_vs_time_graph.png"
            plt.savefig(graph_filename)
            print(f"Graph saved as {graph_filename}")
            plt.show()

            os.remove(plot_file_name)

        except FileNotFoundError as e:
            QMessageBox.critical(self, "Error", str(e))
        except ValueError as e:
            QMessageBox.critical(self, "Error", "Invalid input for test times or slope calculation: " + str(e))

    def read_csv(self, file_path):
        data = []
        with open(file_path, 'r') as file:
            csv_reader = csv.reader(file)
            next(csv_reader)  # Skip the header row
            for row in csv_reader:
                data.append(row)
        return data

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = PressureAnalysisUI()
    window.show()
    sys.exit(app.exec_()) 