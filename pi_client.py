import RPi.GPIO as GPIO
import socket
import time
import sys

# ---------------------
#     CONFIGURATION
# ---------------------

# GPIO
BUTTON_PIN = 17 # GPIO pin to which the button is connected
LED_RED = 23
LED_GREEN = 24
LED_BLUE = 25

# Network
SERVER_IP = "YOUR IP ADDRESS" # Server IP Address
SERVER_PORT = 10709 # Server port, MUST match the one used in the Server C++ script (just a random number)
MESSAGE = "DEFAULT MESSAGE" # This cpuld be user for advenced functions
ENCODING = "utf-8"

# Timing
CHECK_INTERVAL = 3 # Check connection every 3s
BLINK_INTERVAL = 10 # Idle blink every 10s
DEBOUNCE_DELAY = 0.2 # Button debounce


# ----------------------
#       GPIO SETUP
# ----------------------

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)

GPIO.setup(BUTTON_PIN, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup(LED_RED, GPIO.OUT)
GPIO.setup(LED_GREEN, GPIO.OUT)
GPIO.setup(LED_BLUE, GPIO.OUT)


# -----------------
#   LED FUNCTIONS
# -----------------

def set_rgb(r, g, b):
    """
    Sets the RGB LED color by controlling the GPIO pins for red, green, and blue
    Args:
        r (int): Value for the red channel (1 to turn on, 0 to turn off)
        g (int): Value for the green channel (1 to turn on, 0 to turn off)
        b (int): Value for the blue channel (1 to turn on, 0 to turn off)
    Note:
        This function assumes that the LED is active-low, meaning a LOW signal turns the LED on
        The GPIO pins LED_RED, LED_GREEN, and LED_BLUE must be defined and configured as outputs
    """
    GPIO.output(LED_RED, GPIO.LOW if r else GPIO.HIGH)
    GPIO.output(LED_GREEN, GPIO.LOW if g else GPIO.HIGH)
    GPIO.output(LED_BLUE, GPIO.LOW if b else GPIO.HIGH)

def turn_off_led():
    """
    Turns off the RGB LED by setting all channels (red, green, blue) to 0
    Uses the set_rgb function to set the pin values
    """
    set_rgb(0, 0, 0)

def blink_led(r, g, b, times=3, delay=0.2):
    """
    Blinks an RGB LED a specified number of times with the given color and duration

    Args:
        r (int): 0 (off) or 1 (on) for the red channel
        g (int): 0 (off) or 1 (on) for the green channel
        b (int): 0 (off) or 1 (on) for the blue channel
        times (int, optional): Number of times to blink the LED. Defaults to 3
        delay (float, optional): Duration (in seconds) for each blink on and off. Defaults to 0.2
    """
    for _ in range(times):
        set_rgb(r, g, b)
        time.sleep(delay)
        turn_off_led()
        time.sleep(delay)


# ---------------------
#   SERVER CONNECTION
# ---------------------

def connect_to_server():
    # Try to connect until successful connection
    while True:
        try:
            # Attempt connection
            print(f"- Attempting to connect to server: {SERVER_IP}:{SERVER_PORT}")
            set_rgb(1, 1, 0) # Yellow led (attempting connection)
            
            # Create a new TCP/IP socket object for communication with the server
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(10) # Initial connection timeout
            
            sock.connect((SERVER_IP, SERVER_PORT))
            sock.settimeout(None) # Remove timeout after successful connection (blocking mode)
            
            blink_led(0, 1, 0) # Green blink (Connectino established)
            print("> Connection with the server established!")
            
            # Returns the connected socket
            return sock
        except Exception as e:
            # Connection error
            print(f"> Connection error: {e}")
            blink_led(1, 0, 0, 5) # Blink red (5 times)
            time.sleep(3)


# -------------
#   MAIN LOOP
# -------------

def main():
    sock = connect_to_server() # Connect before starting the loop
    last_check = time.time() # Save the current time as the last connection check time
    last_blink = time.time() # Save the current time as the last idle blink time
    
    try:
        while True:
            # If the button is pressed
            if GPIO.input(BUTTON_PIN) == GPIO.LOW:
                try:
                    sock.sendall(MESSAGE.encode(ENCODING)) # Send the message
                    print("> Button pressed! Message sent.") # Print the state inside the Raspberry Terminal
                    
                    blink_led(0, 1, 0, 3, 0.1) # Blink green (3 times, 0.1 delay)
                    last_blink = time.time() # Update the last blink time
                except Exception as e:
                    print(f"> Error sending: {e}") # Print the error inside the Raspberry Terminal
                    sock.close() # Close the connection
                    sock = connect_to_server() # Start a new connection with the server
                
                time.sleep(DEBOUNCE_DELAY) # Debouncing: Delay and wait for button release
                while GPIO.input(BUTTON_PIN) == GPIO.LOW:
                    time.sleep(0.05)
            
            
            # Connection check
            if time.time() - last_check >= CHECK_INTERVAL:
                try:
                    sock.sendall(b'') # Null byte
                    last_check = time.time() # Update the last connection check time
                except Exception as e:
                    print(f"> Connection lost! Error: {e}") # Print the error inside the Raspberry Terminal
                    sock.close() # Close the connection
                    sock = connect_to_server() # Start a new connection with the server
            
            
            # Blink idle
            if time.time() - last_blink >= BLINK_INTERVAL:
                blink_led(0, 0, 1) # Blink blue (3 times)
                last_blink = time.time() # Update the last blink time


            time.sleep(0.05)
    except KeyboardInterrupt: # Script stopped by Keyboard from user
        print("\n> Script terminated by user. Bye :3")
    finally:
        turn_off_led() # Prevent the LED from staying on if the script is interrupted
        GPIO.cleanup() # Restores the state of the GPIO pins and releases resources before exiting
        sock.close() # Close the connection

if __name__ == "__main__":
    # Check if the IP address has been entered
    if SERVER_IP == "YOUR IP ADDRESS":
        print("> YOU MUST set the host IP before running the script!")
        blink_led(1, 0, 0, 10, 0.1) # Blink red (10 times with 0.1s delay)
        sys.exit(1) # Stop the script execution.
    main() # Run the main loop