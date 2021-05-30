# --------------------------------------------------------------------------------------------------------------------
# Repository:
# - Github: https://github.com/rahmant3/polymath-iot
#
# Description:
#   Utility script used to collect CSV training data from a Polymath system with the Nordict UART service.
#
#   Requires the following:
#   - Python 3.8 (note that Python 3.9 does not work)
#       - e.g. Create a virtual environment and activate it:
#            py -3.8 -m venv v
#            env\Scripts\activate.bat
#   - adafruit-circuitpython-ble library
#     - i.e. Using 
#           pip install adafruit-circuitpython-ble
# --------------------------------------------------------------------------------------------------------------------

# Reference: https://learn.adafruit.com/circuitpython-ble-libraries-on-any-computer/ble-uart-example

# --------------------------------------------------------------------------------------------------------------------
# IMPORTS
# --------------------------------------------------------------------------------------------------------------------
from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService

from adafruit_ble.characteristics.stream import StreamOut, StreamIn
from adafruit_ble.uuid import VendorUUID
    
import json
import signal
import time

# --------------------------------------------------------------------------------------------------------------------
# CLASSES
# --------------------------------------------------------------------------------------------------------------------

class UARTServiceCustom(UARTService):
    # Make the UARTService TX/RX timeout longer so that we can receive a full JSON config without having to send it
    # out too quickly.    
    def __init__(self, service=None):
        _server_tx = StreamOut(
            uuid=VendorUUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E"),
            timeout=5.0,
            buffer_size=512,
        )
        _server_rx = StreamIn(
            uuid=VendorUUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E"),
            timeout=5.0,
            buffer_size=512,
        )

        super().__init__(service=service)


class PolymathTraining():
    """ Core class used for collecting data from the Polymath system."""

    def __init__(self, filename="out.csv"):
        self.filename = filename

        self.ble = BLERadio()
        self.uart_connection = None
        
        self.config = None
        self.sample_rate = 0 # In Hz
        self.samples_per_packet = 0
        self.columns = []

        self.sequence = 0
        self.data = []
        pass

    def connect(self):
        result = False 

        if not self.uart_connection:
            for adv in self.ble.start_scan(ProvideServicesAdvertisement):
                if UARTServiceCustom in adv.services:
                    print("Found a device to connect to: {0}".format(adv))
                    self.uart_connection = self.ble.connect(adv, timeout=30)
                    print("Connected Successfully")

                    result = True
                    break
            
            self.ble.stop_scan()

        return result

    def connected(self):
        return (self.uart_connection and self.uart_connection.connected)

    def disconnect(self):
        self.uart_connection.disconnect()

    def run(self):
        if self.connected():
            uart_service = self.uart_connection[UARTServiceCustom]
            if (uart_service.in_waiting > 0):
                line = uart_service.readline().decode()
                print(line)

                if ((line is not None) and (len(line) > 0)):
                    if not self.config:
                        # First try getting the configuration for the system.

                        try:
                            read_config = json.loads(line)
                            print(read_config)
                            print("Parsed JSON successfully")
                            
                            # for key in read_config:
                            #     print("Key: {0}, Value {1}".format(key, read_config[key]))

                            if (    ("sample_rate" in read_config) 
                                    and ("samples_per_packet" in read_config) 
                                    and ("column_location" in read_config)):

                                self.sample_rate = read_config["sample_rate"]
                                self.samples_per_packet = read_config["samples_per_packet"]

                                self.column_locations = []
                                for i in range(0, self.samples_per_packet):
                                    self.column_locations.append("")

                                for key in read_config["column_location"]:
                                    self.column_locations[read_config["column_location"][key]] = key

                                self.config = read_config
                                print("Read a valid config. Columns are {0}".format(self.column_locations))

                                # Initiate a connection
                                uart_service.write("connect".encode())

                                # Set up the output file
                                with open(self.filename, "w") as f:
                                    f.write("sequence,{0}\n".format(','.join(self.column_locations)))
                            else:
                                print("Not all of the expected keys were present.")
                                
                        except ValueError as e:
                            print("Failed to decode JSON due to: {0}".format(e))
                            pass

                    else:
                        # Collect data.

                        data = line.strip().split(',')
                        packet = []

                        add_packet = True #Assume
                        
                        if (len(data) == self.samples_per_packet):
                            for sample in data:
                                try:
                                    packet.append(int(sample))
                                except ValueError:
                                    add_packet = False
                                    break
                        else:
                            add_packet = False

                        if (add_packet):
                            self.data.append(data)
                        
                            with open(self.filename, "a") as f:
                                f.write("{0},{1}\n".format(self.sequence, ','.join(data)))
                                
                            print("Appended {0}".format(data))

                            self.sequence += 1

                        pass


# --------------------------------------------------------------------------------------------------------------------
# MAIN ENTRY POINT
# --------------------------------------------------------------------------------------------------------------------

timestr = time.strftime("%Y_%m_%d-%H_%M_%S")
p = PolymathTraining("{0}_training.csv".format(timestr))

# Set up a signal handler to disconnect on Ctrl-C.
def signal_handler(sig, frame):
    print("Disconnecting...")
    p.disconnect()

signal.signal(signal.SIGINT, signal_handler)

print("Running data collection software. Use Ctrl-C to exit.")

# Start connection
while not p.connected():
    p.connect()

# Start data collection
print("Starting data collection.")
while p.connected():
    p.run()

