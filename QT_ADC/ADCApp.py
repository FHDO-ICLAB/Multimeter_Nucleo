import sys
import serial

from PyQt5 import QtWidgets
from PyQt5.QtWidgets import QDialog, QApplication, QMainWindow 
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

class ADC(QMainWindow):

   channelSelect = 1
   size = 0.0

   def __init__(self):
       super(ADC,self).__init__()
       loadUi("ADC.ui" , self)
       self.OversamplingSection.setEnabled(0)
       self.ResolutionSection.setEnabled(0)
       self.ChannelSection.setEnabled(0)
       self.Connect.clicked.connect(self.Connectfunction)
       self.Disconnect.clicked.connect(self.Disconnectfunction)
       self.Selectedchannel.clicked.connect(self.CurrentChannelFunctio)
       self.ADCValue.clicked.connect(self.ADCValuefunction)
       self.ClearTextChannel.clicked.connect(self.ClearFunction)
       self.ClearTextResolution.clicked.connect(self.ClearFunction)
       self.ClearTextAperture.clicked.connect(self.ClearFunction)
       self.CleartextADC.clicked.connect(self.ClearFunction)

       self.Channel1.clicked.connect(self.SelectChannelFunction)
       self.Channel2.clicked.connect(self.SelectChannelFunction)
       self.Channel3.clicked.connect(self.SelectChannelFunction)
       self.Channel4.clicked.connect(self.SelectChannelFunction)
       self.Channel5.clicked.connect(self.SelectChannelFunction)
       self.Channel11.clicked.connect(self.SelectChannelFunction)
       self.Channel12.clicked.connect(self.SelectChannelFunction)
       self.Channel13.clicked.connect(self.SelectChannelFunction)
                   
       self.Vrefint.clicked.connect(self.vref) 

       self.GetResolution.clicked.connect(self.ADCGetResolutionFunction)
       self.GetResolution_Max.clicked.connect(self.ADCGetMaxResolutionFunction)
       self.GetResolution_Min.clicked.connect(self.ADCGetMinResolutionFunction)
       self.Bit6.clicked.connect(self.ADCSETResolutionFunction)
       self.Bit8.clicked.connect(self.ADCSETResolutionFunction)
       self.Bit10.clicked.connect(self.ADCSETResolutionFunction)
       self.Bit12.clicked.connect(self.ADCSETResolutionFunction)
       self.Max.clicked.connect(self.ADCSETLimitResolutionFunction)
       self.Min.clicked.connect(self.ADCSETLimitResolutionFunction)

       self.EnableOversampling.clicked.connect(self.EnableOversamplingFunction)
       self.DisableOversampling.clicked.connect(self.DisableOversamplingFunction)
       self.GetAperture.clicked.connect(self.GetApertureFunction)
       self.GetAperture_Max.clicked.connect(self.GetAperture_MaxFunction)
       self.GetAperture_Min.clicked.connect(self.GetAperture_MinFunction)
       self.AP49.clicked.connect(self.SETApertureFunction)
       self.AP98.clicked.connect(self.SETApertureFunction)
       self.AP196.clicked.connect(self.SETApertureFunction)
       self.AP392.clicked.connect(self.SETApertureFunction)
       self.AP784.clicked.connect(self.SETApertureFunction)
       self.AP1568.clicked.connect(self.SETApertureFunction)
       self.AP3136.clicked.connect(self.SETApertureFunction)
       self.AP6272.clicked.connect(self.SETApertureFunction)
       self.APmax.clicked.connect(self.SETApertureFunction)
       self.APmin.clicked.connect(self.SETApertureFunction)


       self.textBrowser_1.append(">>WelCome To ADC Configuration Application!")
       self.textBrowser_1.append(">>Please Connect to Serial Port to Continue.") 
   
   def SelectChannelFunction(self):
       Channel = self.sender().objectName()
       channel_number = Channel[7:]
       Command = f"SENS:CHAN {channel_number}" + "\n"
       ser.write(Command.encode())
       receivedLine = str(ser.readline())
       N = len(receivedLine)
       self.TextChannel.append(">>" + receivedLine[2:N])

   def CurrentChannelFunctio(self):
       Command = "SENSe:CHANnel?"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextChannel.append(">>" + receivedLine2[2:N])  

   def vref(self):
       Command = "SENS:CHAN VREFINT"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextChannel.append(">>" + receivedLine2[2:N])  

   def ADCValuefunction(self):
       Command = "MEAS:VOLT?"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.textADC.append(">>" + receivedLine2[2:N])
       
   def ADCGetResolutionFunction(self):
       Command = "SENSe:RESOlution?"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextResolution.append(">>" + receivedLine2[2:N])

   def ADCGetMaxResolutionFunction(self):
       Command = "SENS:RESO? MAX"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextResolution.append(">>" + receivedLine2[2:N]) 

   def ADCGetMinResolutionFunction(self):
       Command = "SENS:RESO? MIN"+ "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextResolution.append(">>" + receivedLine2[2:N])       

   def ADCSETResolutionFunction(self):
       Resolution = self.sender().objectName()
       Resolution_bits = Resolution[3:]
       Command = f"SENSe:RESOlution {Resolution_bits}" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextResolution.append(">>" + receivedLine2[2:N])     

   def ADCSETLimitResolutionFunction(self):
       Resolution = self.sender().objectName()
       Command = f"SENS:RESO {Resolution}" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextResolution.append(">>" + receivedLine2[2:N])  

   def EnableOversamplingFunction(self):
       Command = "Enable_Oversampling" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.OversamplingSection.setEnabled(1)
       self.TextAperture.append(">>" + receivedLine2[2:N])   

   def DisableOversamplingFunction(self):
       Command = "Disable_Oversampling" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.OversamplingSection.setEnabled(0)
       self.TextAperture.append(">>" + receivedLine2[2:N])

   def GetApertureFunction(self):
       Command = "SENS:APER?" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextAperture.append(">>" + receivedLine2[2:N])

   def GetAperture_MaxFunction(self):
       Command = "SENS:APER? Max" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextAperture.append(">>" + receivedLine2[2:N])

   def GetAperture_MinFunction(self):
       Command = "SENS:APER? Min" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextAperture.append(">>" + receivedLine2[2:N])  

   def SETApertureFunction(self):
       Aperture = self.sender().objectName()
       Aperture_number = Aperture[2:]
       Command = f"SENS:APER {Aperture_number}" + "\n"
       ser.write(Command.encode())
       receivedLine2 = str(ser.readline())
       N = len(receivedLine2)
       self.TextAperture.append(">>" + receivedLine2[2:N])  

   def Connectfunction(self):       
       try:
        ser.open()
       except:
        self.textBrowser_1.append(">>Could not open COM port!")
        
       if ser.isOpen():
        self.Connect.setEnabled(0)
        self.Disconnect.setEnabled(1) 
        self.textADC.setEnabled(1)
        self.ADCValue.setEnabled(1)
        self.ResolutionSection.setEnabled(1)
        self.ChannelSection.setEnabled(1)

        self.textBrowser_1.append(">>Board is connected!")        
      
   def Disconnectfunction(self):
       if ser.isOpen():
         try:
          ser.close()  
         except:
          self.textBrowser_1.append(">>Could not Close COM port!")
       if not(ser.isOpen()): 
        self.Connect.setEnabled(1)
        self.Disconnect.setEnabled(0) 
        self.textADC.setEnabled(0)
        self.ADCValue.setEnabled(0)
        self.OversamplingSection.setEnabled(0)
        self.ResolutionSection.setEnabled(0)
        self.ChannelSection.setEnabled(0)
        self.textBrowser_1.append(">>Board is Disconnected!")


   def Channel2function(self):
       self.channelSelect = 2
       self.textBrowser.append(">>Output2 Selected!")
       Command = "INSTrUMEnt:SELect:OUTPut2"
       ser.write(Command.encode())

   def ClearFunction(self):
       Text=self.sender().objectName()
       text_browser_name  = Text[5:]
       text_browser = getattr(self, text_browser_name)
       text_browser.clear() 

       	 	 
app = QApplication(sys.argv)
mainwindow = ADC()
widget = QtWidgets.QStackedWidget()
widget.addWidget(mainwindow)
widget.setWindowTitle("ADC Software Application")
widget.setFixedSize(1065, 670)
widget.show()
app.exec_()


