import sys
import serial

from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QDialog, QApplication 
from PyQt5.uic import loadUi

ser=serial.Serial()
ser.port = 'COM3'
ser.baudrate = 115200
ser.parity   = serial.PARITY_NONE
ser.stopbits = serial.STOPBITS_ONE
ser.bytesize = serial.EIGHTBITS
ser.timeout = 1       #non-block read
ser.xonxoff=False     #disable software flow control
ser.rtscts= False     #disable hardware (RTS/CTS) flow control
ser.dsrdtr= False     #disable hardware (DSR/DTR) flow control
ser.writeTimeout = 2  #timeout for write

class DAC(QDialog):

   channelSelect = 1
   size = 0.0

   def __init__(self):
       super(DAC,self).__init__()
       loadUi("QtDAC.ui",self)
       
       self.ConnectButton.clicked.connect(self.Connectfunction)
       self.DisconnectButton.clicked.connect(self.Disconnectfunction)
       self.Channel1Button.clicked.connect(self.Channel1function)
       self.Channel2Button.clicked.connect(self.Channel2function)
       self.ApplyButton.clicked.connect(self.Applyfunction)             
       self.OutPutQButton.clicked.connect(self.OutPutQfunction) 
       self.VoltageQButton.clicked.connect(self.VoltageQfunction)           

       self.Slider.setMinimum(0)
       self.Slider.setMaximum(4096) 
       self.Slider.setTickInterval(1)
       self.Slider.valueChanged.connect(self.valuechange)

       self.DisconnectButton.setEnabled(0)
       self.Channel1Button.setEnabled(0)
       self.Channel2Button.setEnabled(0)
       self.Slider.setEnabled(0)
       self.Slider.setEnabled(0)
       self.ApplyButton.setEnabled(0)
       self.OutPutQButton.setEnabled(0)
       self.VoltageQButton.setEnabled(0)
     
       self.textBrowser.append(">>WelCome To DAC Configuration Application!")
       self.textBrowser.append(">>Please Connect to Serial Port to Continue.") 
   
   def OutPutQfunction(self):
       Command = "INSTrument:SELect?\n" 
       ser.write(Command.encode())
       receivedLine = str(ser.readline())
       N = len(receivedLine) - 3
       self.textBrowser.append(">>" + receivedLine[2:N])

   def VoltageQfunction(self):
       Command = "SOURce:VOLTage:LEVel?\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2) - 3
       self.textBrowser.append(">>" + receivedLine2[2:N])  
       
   def valuechange(self):
       self.size = self.Slider.value()
       self.size = self.size/4096
       self.size = self.size*3.3
       self.size=round(self.size,4)
       self.label_5.setText(str(self.size))
   
   def Connectfunction(self):       
       try:
        ser.open()
       except:
        self.textBrowser.append(">>Could not open COM port!")
        
       if ser.isOpen():
        self.ConnectButton.setEnabled(0)
        
        self.DisconnectButton.setEnabled(1) 
        self.Channel1Button.setEnabled(1)
        self.Channel2Button.setEnabled(1)
        self.Slider.setEnabled(1)
        self.Slider.setEnabled(1)
        self.ApplyButton.setEnabled(1)
        self.OutPutQButton.setEnabled(1)
        self.VoltageQButton.setEnabled(1)
        self.textBrowser.append(">>Board is connected!")        
      
   def Disconnectfunction(self):
       if ser.isOpen():
         try:
          ser.close()  
         except:
          self.textBrowser.append(">>Could not Close COM port!")
       if not(ser.isOpen()): 
        self.ConnectButton.setEnabled(1)
        self.DisconnectButton.setEnabled(0)
        self.Channel1Button.setEnabled(0)
        self.Channel2Button.setEnabled(0)
        self.Slider.setEnabled(0)
        self.Slider.setEnabled(0)
        self.ApplyButton.setEnabled(0)
        self.OutPutQButton.setEnabled(0)
        self.VoltageQButton.setEnabled(0)
        self.textBrowser.append(">>Board is Disconnected!")

   def Channel1function(self):
       self.channelSelect = 1
       self.textBrowser.append(">>Output1 Selected!")
       Command = "INSTrument:SELect:OUTPut1\n"
       ser.write(Command.encode())

   def Channel2function(self):
       self.channelSelect = 2
       self.textBrowser.append(">>Output2 Selected!")
       Command = "INSTrument:SELect:OUTPut2\n"
       ser.write(Command.encode())

   def Applyfunction(self):     
       SourceVoltageLevel = "SOURce:VOLTage:LEVel:" + str(self.size) + "\n"
       ser.write(SourceVoltageLevel.encode())
       self.textBrowser.append(">>"+ str(self.size) + "V Applied!")
       	 	 
app = QApplication(sys.argv)
mainwindow = DAC()
widget = QtWidgets.QStackedWidget()
widget.addWidget(mainwindow)
widget.setWindowTitle("DAC Qt Software Application")
widget.setFixedSize(373,500)
widget.show()
app.exec_()


