import RPi.GPIO as GPIO
import time
import socket

# Button GPIO Config
BUTTON_PIN = 17 # GPIO pin to which the button is connected
GPIO.setmode(GPIO.BCM) # Sets the pin numbering mode to BCM (Broadcom SOC channel)
GPIO.setup(BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP) # Set up BUTTON_PIN as input with an internal pull-up resistor

# RGB LED GPIO Config
LED_RED_PIN = 23
LED_GREEN_PIN = 24
LED_BLUE_PIN = 25

GPIO.setup(LED_RED_PIN, GPIO.OUT)
GPIO.setup(LED_GREEN_PIN, GPIO.OUT)
GPIO.setup(LED_BLUE_PIN, GPIO.OUT)

# Network configuration
SERVER_HOST = "YOUR HOST IP" # Server IP Address
SERVER_PORT = 10709 # Server port, MUST match the one used in the Server C++ script (just a random number)
MESSAGE = "BTN_PRESSED" # Default message, change it for custom actions
MESSAGE_ENCODE = 'utf-8' # Encoding used to convert the message string to bytes before sending

# Global socket variable
s = None

# Idle blink variables
last_idle_blink_time = time.time()
IDLE_BLINK_INTERVAL = 10 # Time in seconds

# Timer for periodic connection check during idle
last_connection_check_time = time.time()
CONNECTION_CHECK_INTERVAL = 10 # Time in seconds



def set_rgb_color(red, green, blue):
    """
    Sets the RGB LED color by controlling the GPIO pins for red, green, and blue.
    Args:
        red (int): Value for the red channel (1 to turn on, 0 to turn off).
        green (int): Value for the green channel (1 to turn on, 0 to turn off).
        blue (int): Value for the blue channel (1 to turn on, 0 to turn off).
    Note:
        This function assumes that the LED is active-low, meaning a LOW signal turns the LED on.
        The GPIO pins LED_RED_PIN, LED_GREEN_PIN, and LED_BLUE_PIN must be defined and configured as outputs.
    """

    GPIO.output(LED_RED_PIN, GPIO.LOW if red == 1 else GPIO.HIGH)
    GPIO.output(LED_GREEN_PIN, GPIO.LOW if red == 1 else GPIO.HIGH)
    GPIO.output(LED_BLUE_PIN, GPIO.LOW if red == 1 else GPIO.HIGH)


def turn_off_rgb():
    """
    Turns off the RGB LED by setting all channels (red, green, blue) to 0.
    Uses the set_rgb_color function to set the pin values.
    """
    set_rgb_color(0, 0, 0)


def n_blink_led(red, green, blue, n_blinks = 3, blink_duration = 0.2):
    """
    Blinks an RGB LED a specified number of times with the given color and duration.

    Args:
        red (int): 0 (off) or 1 (on) for the red channel.
        green (int): 0 (off) or 1 (on) for the green channel.
        blue (int): 0 (off) or 1 (on) for the blue channel.
        n_blinks (int, optional): Number of times to blink the LED. Defaults to 3.
        blink_duration (float, optional): Duration (in seconds) for each blink on and off. Defaults to 0.2.
    """
    for _ in range(n_blinks):
        set_rgb_color(red, green, blue)
        time.sleep(blink_duration)
        turn_off_rgb()
        time.sleep(blink_duration)


def connect_to_server():
    global s

    # Try to connect until successful connection
    while True:
        print(f"- Attempting to connect to the server {SERVER_HOST}:{SERVER_PORT}")

        # Turns on the red LED to indicate a connection attempt or connection error
        set_rgb_color(1, 0, 0)

        try:
            # Create a new TCP/IP socket object for communication with the server
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(10) # Initial connection timeout
            s.connect((SERVER_HOST, SERVER_PORT))

            # Remove timeout after successful connection (blocking mode)
            s.settimeout(None)

            print("> Connection with the server established!")
            
            triple_blink(0, 1, 0) # Green blink
            last_idle_blink_time = time.time() # Update the last blink time

            # Returns the connected socket
            return s
        except socket.error as e:
            print(f"> Connection error: {e}. Retrying in 5 seconds.")
            set_rgb_color(1, 0, 0) # Set the RGB LED to red
            time.sleep(5)
        except Exception as e:
            print("> Unexpected error during connection: {e}. Retrying in 5 seconds.")
            set_rgb_color(1, 0, 0) # Set the RGB LED to red
            time.sleep(5)


def throw_socket_error(message, error = "", red = 1, green = 0, blue = 0, blink_times = 5, blink_duration = 0.2):
    """
    Handles socket errors by printing an error message, blinking an LED with specified color and pattern, and closing the socket.

    Args:
        message (str): The error message to display.
        error (str, optional): Additional error details. Defaults to "".
        red (int, optional): Red component of the LED color (1 to turn on, 0 to turn off). Defaults to 1.
        green (int, optional): Green component of the LED color (1 to turn on, 0 to turn off). Defaults to 0.
        blue (int, optional): Blue component of the LED color (1 to turn on, 0 to turn off). Defaults to 0.
        blink_times (int, optional): Number of times the LED should blink. Defaults to 5.
        blink_duration (float, optional): Duration of each blink in seconds. Defaults to 0.2.

    Side Effects:
        Prints the error message to the console.
        Blinks the LED with the specified color and pattern.
        Closes the socket connection.
    """
    print(f"> {message} {error}")
    n_blink_led(red, green, blue, blink_times, blink_duration)
    s.close()
    s = None

# Main script loop
turn_off_rgb()

try:
    while True:
        # Check for socket connection
        if s is None:
            s = connect_to_server() # Try to connect
        
        input_state = GPIO.input(BUTTON_PIN)

        # Button pressed
        if input_state == False:
            try:
                # Try to send a null byte to check the connection
                s.sendall(b'')

                # Send the message
                s.sendall(MESSAGE.encode(MESSAGE_ENCODE))
                print(f"> Message sent: '{MESSAGE}'")

                set_rgb_color(0, 1, 0) # Green led
                time.sleep(2)
                turn_off_rgb()
            except socket.error as e:
                throw_socket_error("Error sending. Connection lost or server not responding. Error:", e)
            except Exception as e:
                print(f"> Generic error during sending. Error: {e}")

                n_blink_led(1, 0, 0, 5) # Blink 5 time with red

                s.close()
                s = None
            
            # Reset the idle blink timer
            last_idle_blink_time = time.time()

            # Debouncing: Delay and wait for button release
            time.sleep(0.2)
            while GPIO.input(BUTTON_PIN) == False: # Wait for the button release
                time.sleep(0.1)
        else: # Idle state (button not pressed)
            # If the socket is connected
            if s is not None and (time.time() - last_connection_check_time) >= CONNECTION_CHECK_INTERVAL:
                try:
                    s.sendall(b'') # Send an empty byte
                except socket.error as e:
                    throw_socket_error("Error during periodic check. Connection lost or server not responding. Error:", e)
                
                last_connection_check_time = time.time() # Update the check timer

                # Check if connected and if it's time to perform the idle blink
                if s is not None and (time.time() - last_idle_blink_time) >= IDLE_BLINK_INTERVAL:
                    n_blink_led(0, 0, 1) # Blue blink
                    last_idle_blink_time = time.time() # Update the time since the last blink
        
        time.sleep(0.05) # Small delay to reduce the CPU usage
except KeyboardInterrupt:
    print("\n> Script terminated by user.")
finally:
    # Turn off the RGB
    turn_off_rgb()

    # Close the socket connection if open
    if s:
        s.close()
    
    GPIO.cleanup() # Restores the state of the GPIO pins and releases resources before exiting