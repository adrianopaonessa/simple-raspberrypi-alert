# 🚨 Simple Alert System 🚨
###### By [Adriano Paonessa](https://github.com/adrianopaonessa/)

A simple notification system based on Raspberry Pi and a Windows PC to alert you when something requires attention.

---

## 📄 Table of Contents

* [✨ Features](#-features)
* [📦 Required Components](#-required-components)
* [📐 System Architecture](#-system-architecture)
* [🚀 Installation and Configuration Guide](#-installation-and-configuration-guide)
    * [1. Hardware Setup (Raspberry Pi)](#1-hardware-setup-raspberry-pi)
    * [2. Software Setup (Windows PC)](#2-software-setup-windows-pc)
    * [3. Software Setup (Raspberry Pi)](#3-software-setup-raspberry-pi)
    * [4. Final Test](#4-final-test)
* [🤝 Contribution](#-contribution)
* [📄 License](#-license)
* [✉️ Contact](#️-contact)

---

### ✨ Features

* **Instant Notification:** Sends a message to your Windows PC when a button is pressed on the Raspberry Pi.
* **Intuitive Visual Feedback:** The RGB LED on the Raspberry Pi indicates the system's status:
    * **Off:** Stable connection, system idle.
    * **Triple Blue Blink:** System idle but connected, indicating it's operational (every 10 seconds).
    * **Solid Red:** Connection to the PC lost or in reconnection phase.
    * **Temporary Green:** Message successfully sent.
    * **Temporary Red:** Error during message sending.
* **Persistent Connection:** The Raspberry Pi maintains an active, persistent TCP/IP connection with the server.
* **Automatic Reconnection:** The Pi automatically attempts to re-establish the connection if it's lost.
* **Auto-Start on Boot:** Both components (server on PC and client on Pi) automatically launch when their respective devices power on.
* **Tray Icon Management:** The Windows server now includes a tray icon for easy application management, including showing/hiding the console or stopping the service.

---

### 📦 Required Components

* **Hardware:**
    * Raspberry Pi (any model with GPIO pins)
    * RGB LED (Common Anode)
    * Push Button
    * Resistors (for the RGB LED, typically around 220 Ohm)
    * Breadboard and jumper wires
* **Software:**
    * Raspberry Pi OS (or a compatible Linux distribution)
    * Python 3 (pre-installed on Raspberry Pi OS)
    * `RPi.GPIO` library (pre-installed)
    * Windows 10/11 PC
    * C++ Compiler (e.g., MinGW or MSVC)

---

### 📐 System Architecture

The system consists of two main parts:

1.  **Client (Raspberry Pi):**
    * Monitors the state of a physical button connected to its GPIO pins.
    * Manages visual feedback and system status through an RGB LED.
    * Establishes and maintains a persistent TCP/IP connection with the Windows PC server.
    * Sends a predefined "DEFAULT MESSAGE" message to the server when the button is pressed.

2.  **Server (Windows PC):**
    * Runs as a background service.
    * Listens for incoming TCP/IP connections on a specific port.
    * Receives messages from the Raspberry Pi.
    * Triggers a native Windows Toast notification to alert the user.
    * Includes a tray icon for application management: showing/hiding the console log and stopping the service.

---

### 🚀 Installation and Configuration Guide

Follow these steps to set up your Simple Alert system:

#### 1. Hardware Setup (Raspberry Pi)

Connect the Common Anode RGB LED and the push button to your Raspberry Pi's GPIO pins as follows:

* **Button:**
    * One side to **GPIO 17** (Physical Pin 11).
    * The other side to **GND** (Physical Pin 9).
    * *(The Python code uses an internal pull-up resistor, so no external resistor is needed for pull-up).*

* **RGB LED (Common Anode):**
    * **Common Anode** pin (the longest one) to **3.3V** (Physical Pin 1).
    * **Red** pin (with a ~220 Ohm resistor) to **GPIO 23** (Physical Pin 16).
    * **Green** pin (with a ~220 Ohm resistor) to **GPIO 24** (Physical Pin 18).
    * **Blue** pin (with a ~220 Ohm resistor) to **GPIO 25** (Physical Pin 22).

---

#### 2. Software Setup (Windows PC)

1.  **Compile the C++ Server:**
    * Ensure you have a C++ compiler (e.g., MinGW, MSVC) installed on your Windows PC.
    * Compile your `server_notifier.cpp` file to create the `server_notifier.exe` executable.
    * **Move `server_notifier.exe`** to a permanent, dedicated location on your PC, for example, `C:\SimpleAlert\`.
    * **Place the `toast_notification.ps1` script in the same `C:\SimpleAlert\` directory** alongside `server_notifier.exe`.

2.  **Configure Auto-Start and Management:**
    The server now manages its visibility and termination via a tray icon. To have it launch automatically upon Windows startup:

    * **Create a shortcut (`.lnk` file)** to `server_notifier.exe`:
        * Right-click on `server_notifier.exe` -> Select `Create shortcut`.
    * **Move this shortcut** to the Windows Startup folder:
        * Press `Win + R` (Windows key + R) to open the Run dialog.
        * Type `shell:startup` and press Enter to open the current user's Startup folder.
        * *(For all users, you can use `shell:common startup`, but moving files here often requires administrator privileges).*

    Once started, the program will appear as an icon in the Windows system tray (near the clock). You can left-click to show/hide the console window or right-click to access a menu with "Show/Hide console window" and "Stop server notifier" options.

---

### 3. Software Setup (Raspberry Pi)

1.  **Copy the Python Client:**
    * Copy your `pi_client.py` file to your Raspberry Pi. A good location would be within your home directory, e.g., `/home/pi/SimpleAlert/pi_client.py`.

2.  **Update Server IP Address:**
    * Open the `pi_client.py` file using a text editor (e.g., `nano pi_client.py`).
    * Locate the line `SERVER_IP = "YOUR IP ADDRESS"` and **change `"YOUR IP ADDRESS"` to the actual IP address of your Windows PC**.

3.  **Configure Auto-Start for the Client:**
    * To make your Python client script run automatically every time the Raspberry Pi boots, we'll use a `systemd` service.
    * Create a new systemd service file:
        ```bash
        sudo nano /etc/systemd/system/simple_alert.service
        ```
    * Paste the following content into the file. **Make sure to modify the `ExecStart` and `WorkingDirectory` paths** to match the actual location where you saved your `pi_client.py` script.
        ```ini
        [Unit]
        Description=Simple Alert Service for Raspberry Pi
        After=network.target # Ensures the network is up before starting this service

        [Service]
        ExecStart=/usr/bin/python3 /home/pi/SimpleAlert/pi_client.py
        WorkingDirectory=/home/pi/SimpleAlert/
        StandardOutput=inherit # Inherit standard output
        StandardError=inherit  # Inherit standard error (for logging)
        Restart=always         # Restart the service if it stops unexpectedly
        User=pi                # Run the service as the 'pi' user (or your Raspberry Pi username)

        [Install]
        WantedBy=multi-user.target # Start this service in multi-user mode (even without a GUI)
        ```
    * Save the file (`Ctrl+O`, then Enter) and exit (`Ctrl+X`).
    * Now, reload the `systemd` manager to recognize your new service and then enable it to start on boot:
        ```bash
        sudo systemctl daemon-reload
        sudo systemctl enable simple_alert.service
        sudo systemctl start simple_alert.service # Start the service immediately for testing
        ```

---

### 4. Final Test

* After completing all the above steps, **reboot both your Windows PC and your Raspberry Pi**.
* **Verify the C++ server is running** on your Windows PC by checking for the "Raspberry Pi Server Notifier" icon in the system tray (it might be hidden in the overflow menu).
* **Verify the Python client is running** on your Raspberry Pi by running `sudo systemctl status simple_alert.service`. It should show `Active: active (running)`.
* Finally, **press the button** on your Raspberry Pi and observe the LED feedback and the Toast notification appearing on your Windows PC!

---

### 🤝 Contribution

Feel free to clone the repository, explore the code, and suggest improvements or new features via pull requests.

---

### 📄 License

This project is released under the [MIT License](LICENSE).

---

### ✉️ Contact

For questions or suggestions, you can open an issue on this repository.