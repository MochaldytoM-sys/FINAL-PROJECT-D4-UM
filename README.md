# FINAL-PROJECT-D4-UM
ZERO ENERGY STATE VERIFICATION SYSTEM PROTOTYPE ON LOTO BASED ON ESP32 WITH FAIL-SAFE AND LED MATRIX HUB75
# ZERO ENERGY STATE (ZES) Verification System

### ESP32-Based Lockout/Tagout (LOTO) Safety Verification System

> A prototype system for verifying **Zero Energy State (ZES)** in Lockout/Tagout (LOTO) procedures using multi-sensor monitoring, fail-safe logic, RFID authentication, real-time HMI, and digital data logging.

---

## 📌 Project Overview

The **ZERO ENERGY STATE (ZES) Verification System** is an ESP32-based prototype designed to support the verification of hazardous residual energy before maintenance activities in an industrial environment.

The system integrates multiple sensors to detect electrical, pneumatic, and mechanical energy conditions. Sensor data is processed by an **ESP32** and evaluated using a **fail-safe decision-making logic**.

Maintenance access is only permitted when all monitored energy sources are verified to be in a safe condition.

The system also provides:

* RFID-based operator authentication
* Real-time safety status visualization
* Fail-safe and physical interlock
* Audible warning using buzzer
* Data logging using SD Card
* RTC-based timestamping
* Local web server for monitoring and log access

---

## 🎯 Project Objectives

The main objectives of this project are:

1. Develop a prototype for verifying **Zero Energy State** before maintenance.
2. Detect residual energy from multiple energy sources.
3. Implement a **fail-safe mechanism** to prevent maintenance access when unsafe conditions are detected.
4. Provide real-time system status through an **HUB75 LED Matrix**.
5. Implement RFID authentication for operator identification.
6. Record verification activities digitally using SD Card and RTC.
7. Provide local monitoring and log access through an ESP32 web server.

---

## ⚙️ System Architecture

The system uses an ESP32 as the central controller.

```text
                    ┌─────────────────────┐
                    │     RFID MFRC522    │
                    │ Operator Auth.      │
                    └──────────┬──────────┘
                               │
                               ▼
┌─────────────┐        ┌───────────────────┐
│ ZMPT101B    │───────►│                   │
│ Voltage     │        │                   │
├─────────────┤        │                   │
│ MPX5010     │───────►│      ESP32        │
│ Pressure    │        │ Central Controller│
├─────────────┤        │                   │
│ QPM11       │───────►│  ZES Verification │
│ Pressure    │        │                   │
├─────────────┤        │  Fail-Safe Logic  │
│ Limit       │───────►│                   │
│ Switch      │        │                   │
├─────────────┤        │                   │
│ Rotary      │───────►│                   │
│ Encoder     │        └─────────┬─────────┘
└─────────────┘                  │
                                 │
              ┌──────────────────┼──────────────────┐
              │                  │                  │
              ▼                  ▼                  ▼
       ┌─────────────┐    ┌─────────────┐   ┌─────────────┐
       │ LED Matrix  │    │ Servo        │   │ Buzzer      │
       │ HUB75       │    │ Interlock    │   │ Warning     │
       └─────────────┘    └─────────────┘   └─────────────┘
              │
              ▼
       ┌─────────────┐
       │ SD Card +   │
       │ RTC DS3231  │
       └──────┬──────┘
              │
              ▼
       ┌─────────────┐
       │ Local Web   │
       │ Server      │
       └─────────────┘
```

---

## 🔄 System Workflow

The system operates using a verification sequence based on authentication and sensor conditions.

```text
START
  │
  ▼
System Initialization
  │
  ▼
STANDBY
  │
  ▼
RFID Authentication
  │
  ├── Unauthorized ──► Access Denied
  │
  ▼
Energy Verification
  │
  ├── Unsafe ────────► DANGER
  │                       │
  │                       ├── Buzzer ON
  │                       ├── Servo LOCK
  │                       └── Maintenance Denied
  │
  ▼
All Energy Sources SAFE
  │
  ▼
ZERO ENERGY STATE
  │
  ├── LED Matrix: SAFE
  ├── Servo: UNLOCK
  └── Maintenance Allowed
```

---

## 🧠 Finite State Machine

The system implements a state-based control approach.

### Main States

| State                   | Description                                                   |
| ----------------------- | ------------------------------------------------------------- |
| **STANDBY / RFID**      | System waits for registered operator authentication           |
| **ENERGY VERIFICATION** | ESP32 evaluates all connected safety sensors                  |
| **SAFE / ZERO ENERGY**  | All monitored energy sources meet safe conditions             |
| **DANGER**              | Residual or hazardous energy is detected                      |
| **MAINTENANCE**         | Maintenance access is permitted after successful verification |

The fail-safe principle is applied so that unsafe or abnormal conditions prevent maintenance access.

---

## 🔌 Hardware

### Main Controller

* ESP32
* ESP32-S3 PRO

### Energy & Condition Sensors

* ZMPT101B voltage sensor
* Voltage divider
* MPX5010 pressure sensor
* Mechanical pressure switch / QPM11
* Low pressure switch
* Rotary Encoder / Hall Effect Sensor
* NC Limit Switch

### Identification & Data

* RFID MFRC522
* RTC DS3231
* SD Card module

### HMI & Warning

* HUB75 LED Matrix
* Buzzer

### Actuator & Interlock

* Servo motor
* Relay module

---

## 🖥️ Human-Machine Interface

The HUB75 LED Matrix provides real-time visual feedback to the operator.

### RFID

The system initially waits for an authorized RFID card.

```text
RFID
TAP CARD
```

### SAFE

Displayed when all monitored sensors indicate a safe condition.

```text
SAFE
ZERO ENERGY
```

### DANGER

Displayed when residual hazardous energy is detected.

```text
DANGER
ENERGY DETECTED
```

The buzzer is activated and maintenance access remains locked.

### MAINTENANCE

Displayed after successful RFID authentication and verification of all monitored energy sources.

```text
MAINTENANCE
ACCESS GRANTED
```

---

## 🔐 Fail-Safe & Interlock

Safety is implemented using a fail-safe approach.

The system is designed so that:

* Unsafe energy condition → **maintenance denied**
* Sensor abnormality → **system remains in safe/locked condition**
* Hazardous energy detected → **buzzer warning**
* Hazardous energy detected → **DANGER displayed**
* Unsafe condition → **servo remains locked**
* All required conditions safe → **maintenance access permitted**

The servo actuator is configured with a default locked position and only changes to the unlocked position after the required safety conditions have been verified.

---

## 🪪 RFID Authentication

RFID is used to identify the operator before the verification process begins.

The system checks the RFID UID against registered user data stored on the SD Card.

The logging system records information including:

* Sequence number
* Timestamp
* RFID UID
* Operator name
* Access status
* Sensor conditions
* System status
* Maintenance status

---

## 💾 Data Logging

Verification activity is stored in **CSV format** on the SD Card.

The system uses the RTC DS3231 to provide timestamp information.

The stored data can also be accessed through the **local web server hosted by the ESP32**.

The system is designed to automatically remove log files older than **14 days**.

Example data structure:

```text
No,Time,UID,Operator,Access,Sensor_Status,System_Status,Maintenance
1,08:15:23,XXXXXX,Operator_01,GRANTED,SAFE,SAFE,ALLOWED
```

---

## 🌐 Local Web Server

The ESP32 provides a local web interface for monitoring and accessing recorded log data.

The web interface is intended to provide:

* System monitoring
* Log file access
* Log file download
* Digital documentation of verification activities

This allows the verification process to be supported not only by physical indicators but also by digital records.

---

## 🧪 Testing

System testing was divided into three main categories.

### 1. Visualization & Warning System

Testing focuses on:

* HUB75 LED Matrix status
* SAFE status
* DANGER status
* MAINTENANCE status
* Buzzer response
* Response time

The project defines a target response time of **less than 2 seconds** for visual and warning responses.

### 2. Fail-Safe & Interlock

Testing focuses on:

* Residual energy conditions
* Unsafe sensor conditions
* Sensor failure conditions
* Buzzer activation
* Servo lock condition
* Maintenance access prevention

### 3. Identification & Data Logging

Testing focuses on:

* Registered RFID
* Unregistered RFID
* UID identification
* RTC timestamp
* CSV data storage
* SD Card logging
* Local web server access

---

## 📊 Project Results

Based on the project testing and evaluation, the developed prototype was able to:

* Verify Zero Energy State conditions in real time.
* Detect hazardous residual energy using multiple sensors.
* Display **RFID, SAFE, DANGER, and MAINTENANCE** states through the HUB75 LED Matrix.
* Provide audible warnings when hazardous energy is detected.
* Implement operator authentication using RFID.
* Maintain the interlock in a locked condition when unsafe conditions are detected.
* Record verification activity automatically to SD Card.
* Provide access to recorded logs through the ESP32 local web server.

---

## 📷 Prototype Documentation

Project documentation can be found in:

```text
08-documentation/
```

Recommended documentation:

* Prototype overview
* Front view
* Rear view
* Control panel
* Wiring
* Sensor installation
* RFID module
* LED Matrix
* Servo interlock
* Testing process
* Final prototype

---

## 📁 Repository Structure

```text
zero-energy-state-loto/
│
├── README.md
│
├── 01-project-overview/
│   ├── project-summary.md
│   ├── objectives.md
│   └── system-overview.png
│
├── 02-system-design/
│   ├── block-diagram/
│   ├── flowchart/
│   ├── electrical-schematic/
│   └── finite-state-machine/
│
├── 03-hardware/
│   ├── components.md
│   ├── esp32/
│   ├── sensors/
│   ├── rfid/
│   ├── led-matrix/
│   └── actuator/
│
├── 04-software/
│   ├── source-code/
│   ├── libraries.md
│   └── software-architecture.png
│
├── 05-electrical-calculation/
│   ├── calculation.md
│   └── calculation.xlsx
│
├── 06-hmi-and-monitoring/
│   ├── led-matrix/
│   ├── web-interface/
│   └── screenshots/
│
├── 07-testing/
│   ├── test-plan.md
│   ├── visual-warning/
│   ├── fail-safe-interlock/
│   ├── rfid/
│   ├── data-logging/
│   └── test-results.xlsx
│
├── 08-documentation/
│   ├── prototype-front.jpg
│   ├── prototype-back.jpg
│   ├── wiring-process.jpg
│   └── testing-process.jpg
│
└── 09-final-report/
    └── final-report.pdf
```

---

## 🛠️ Engineering Skills Demonstrated

This project demonstrates experience in:

* Embedded System Development
* ESP32 Microcontroller
* Sensor Integration
* Electrical & Electronics Design
* Industrial Safety System
* Lockout/Tagout (LOTO)
* Fail-Safe System Design
* Interlock System
* RFID Authentication
* Human-Machine Interface (HMI)
* LED Matrix HUB75
* Data Logging
* SD Card / CSV Data Management
* RTC Integration
* Local Web Server
* Finite State Machine (FSM)
* System Testing & Validation
* Hardware-Software Integration

---

## 👨‍💻 Project Role

**Role:** Electrical / Embedded System Engineer — Final Project

Responsibilities included:

* System architecture design
* Electrical system design
* Hardware integration
* Sensor integration
* ESP32 programming
* HMI implementation
* Fail-safe logic implementation
* RFID authentication
* Data logging implementation
* System testing
* Prototype evaluation

---

## 📚 Documentation

For detailed technical information, refer to:

```text
09-final-report/final-report.pdf
```

The repository is structured to make the technical design, implementation, and testing easier to review than the original academic report.

---

## 📌 Project Status

**Status:** Completed Prototype

**Focus:** Industrial Safety / Embedded System / Electrical Engineering

**Application:** Zero Energy State Verification for Lockout/Tagout (LOTO)
