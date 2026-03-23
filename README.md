# Delirium-Detection-Device  
### Microcontroller-Based Delirium Detection System using ESP32 & C++  

## 🧠 Problem Statement  
Hospital-induced delirium affects up to **87% of ICU patients**, with nearly **70% of cases remaining undiagnosed** due to infrequent and subjective manual CAM assessments.  

The **Confusion Assessment Method (CAM)** is a standard clinical tool used to identify delirium by evaluating:  
- **Acute Onset** – Sudden change in mental status  
- **Inattention** – Inability to focus or sustain attention  
- **Disorganized Thinking** – Illogical or incoherent responses  
- **Altered Level of Consciousness** – Reduced awareness or alertness  

However, manual CAM assessments vary between clinicians and may overlook early signs of cognitive change.  

---

## 💡 Project Overview  
This project aims to **automate the CAM assessment process** using an **ESP32 microcontroller**.  
- The device presents standardized CAM questions to the patient via a **display** and/or **speaker**.  
- Patients interact using **buttons**, **touch**, or **speech** input.  
- The **Gemini API** generates CAM-based questions dynamically.  
- Responses are stored in an **Excel-compatible data file** for further analysis and tracking.  

---

## ⚙️ Hardware Components  
- ESP32 Microcontroller  
- 3.5" LCD Display  
- Tactile Buttons  
- Joystick with Push Button  
- Speaker & Microphone Module  
- LEDs & Vibration Motors  
- Breadboard / PCB  
- Battery Pack  
- Connecting Wires  

---

## 🧩 Technologies Used  
- **C++** for microcontroller logic  
- **ESP-IDF / Arduino IDE** for firmware development  
- **Gemini API** for dynamic question generation  
- **Excel (via data logging)** for analytics and tracking  


