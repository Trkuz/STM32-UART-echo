# STM32-UART-echo
### Overview
#### This program, initializes an interrupt-driven UART utilizing Transmit and Recieve circular buffers, and software flow control.

**The program operates in two modes**:

* **Auto mode**: The automatic message is displayed on screen every 0.5s.

* **Manual mode (Echo mode)**: The input message is taken from user, and displayed on the output.

---

### Supported Hardware
 * **STM32 F4 series** (Tested on STM32 Nucleo-F446RE)

 #### Requires connection via USB-to-UART (USB-UART adapter or ST-Link virtual COM port)

---

### Software Requirements
* **Putty or other UART terminal software**

* #### Serial settings:
    **-Baud Rate**: 9600  
    **-Data Bits**: 8  
    **-Parity**: None  
    **-Stop Bits**: 1  
    *-Flow Control**: XOFF/XON  
    **-Connection Type**: Serial  
    **-Serial Line**: Serial Line assosciated with your STM32 (Typically it is COM3)  

---

### Usage
1. Flash the program thorugh the ST-Link
2. Connect the board via USB
3. Open the UART termianal software, and cofigure the serial port
4. Enable logging (in Putty go to Session -> Logging -> All session output -> Select a file path to save the log to)
5. (Optional) View logs live in Powershell (Type in: Get-Content "C:\path\to\logfile.log" -Wait)

---

### Additional Macros used in project

#### In additon to main.c file, a few additonal macros have been defined in main.h and are required to ensure the proper work of program:

#define LD2_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()  
#define BUTTON_GPIO_CLK_ENABLE() __HAL_RCC_GPIOC_CLK_ENABLE()  
#define USER_BUTTON_GPIO_Port GPIOC  
#define USER_BUTTON_Pin GPIO_PIN_13 


