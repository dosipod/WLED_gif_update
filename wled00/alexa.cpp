#include "wled.h"

/*
 * Alexa Voice On/Off/Brightness/Color Control. Emulates a Philips Hue bridge to Alexa.
 * 
 * This was put together from these two excellent projects:
 * https://github.com/kakopappa/arduino-esp8266-alexa-wemo-switch
 * https://github.com/probonopd/ESP8266HueEmulator
 */
#include "src/dependencies/espalexa/EspalexaDevice.h"

#ifndef WLED_DISABLE_ALEXA
void onAlexaChange(EspalexaDevice* dev);

void alexaInit()
{
  if (alexaEnabled && WLED_CONNECTED)
  {
    espalexa.removeAllDevices();
    // the original configured device for keeping old behavior (added first, i.e. index 0)
    espalexaDevice = new EspalexaDevice(alexaInvocationName, onAlexaChange, EspalexaDeviceType::extendedcolor);
    espalexa.addDevice(espalexaDevice);
    // up to 9 devices (added second, third, ... i.e. index 1 to 9) serve for switching on up to nine presets (preset IDs 1 to 9 in WLED), 
    // names are identical as the preset names, switching off can be done by switching off any of them
    for (byte presetIndex=1; presetIndex<=9; ++presetIndex) 
    {
      String name = getPresetName(presetIndex);
      if (name == "") break; // no more presets
      espalexaDevice = new EspalexaDevice(name.c_str(), onAlexaChange, EspalexaDeviceType::extendedcolor);
      espalexa.addDevice(espalexaDevice);
    }
    espalexa.begin(&server);
  } 
}

void handleAlexa()
{
  if (!alexaEnabled || !WLED_CONNECTED) return;
  espalexa.loop();
}

void onAlexaChange(EspalexaDevice* dev)
{
  espalexaDevice = dev;
  EspalexaDeviceProperty m = espalexaDevice->getLastChangedProperty();
  String name = espalexaDevice->getName();
  
  if (m == EspalexaDeviceProperty::on)
  {
    if (name == alexaInvocationName) 
    {
      // keep the old switch-on behavior for the configured name
      if (!macroAlexaOn)
      {
        if (bri == 0)
        {
          bri = briLast;
        stateUpdated(CALL_MODE_ALEXA);
        }
      } else 
      {
        applyPreset(macroAlexaOn, CALL_MODE_ALEXA);
        if (bri == 0) espalexaDevice->setValue(briLast); //stop Alexa from complaining if macroAlexaOn does not actually turn on
      }
    } else 
    {
      // new switch-on behavior for preset devices
      byte preset = 0;
      // find the index with the right name, leave out index 0 (device with alexaInvocationName does not occur in this else branch)
      for (byte alexaIndex=1; alexaIndex<espalexa.getDeviceCount(); ++alexaIndex)
      {
        if (name == espalexa.getDevice(alexaIndex)->getName())
        {
          preset = alexaIndex; // in alexaInit() preset 1 device was added second (index 1), preset 2 third (index 2) etc.
        } else
        {
          espalexa.getDevice(alexaIndex)->setValue(0); // set other presets off
        }
      }
      applyPreset(preset,CALL_MODE_ALEXA);
      espalexa.getDevice(0)->setValue(espalexaDevice->getValue()); // set alexaInvocationName device to the current value
    }
  } else if (m == EspalexaDeviceProperty::off)
  {
    if (!macroAlexaOff)
    {
      if (bri > 0)
      {
        briLast = bri;
        bri = 0;
        stateUpdated(CALL_MODE_ALEXA);
      }
    } else 
    {
      applyPreset(macroAlexaOff, CALL_MODE_ALEXA);
      if (bri != 0) espalexaDevice->setValue(0); //stop Alexa from complaining if macroAlexaOff does not actually turn off
    }
    for (byte alexaIndex=0; alexaIndex<espalexa.getDeviceCount(); ++alexaIndex)
    {
      espalexa.getDevice(alexaIndex)->setValue(0);
    }
  } else if (m == EspalexaDeviceProperty::bri)
  {
    bri = espalexaDevice->getValue();
    stateUpdated(CALL_MODE_ALEXA);
  } else //color
  {
    if (espalexaDevice->getColorMode() == EspalexaColorMode::ct) //shade of white
    {
      byte rgbw[4];
      uint16_t ct = espalexaDevice->getCt();
			if (!ct) return;
			uint16_t k = 1000000 / ct; //mireds to kelvin
			
			if (strip.hasCCTBus()) {
				strip.setCCT(k);
				rgbw[0]= 0; rgbw[1]= 0; rgbw[2]= 0; rgbw[3]= 255;
			} else if (strip.hasWhiteChannel()) {
        switch (ct) { //these values empirically look good on RGBW
          case 199: rgbw[0]=255; rgbw[1]=255; rgbw[2]=255; rgbw[3]=255; break;
          case 234: rgbw[0]=127; rgbw[1]=127; rgbw[2]=127; rgbw[3]=255; break;
          case 284: rgbw[0]=  0; rgbw[1]=  0; rgbw[2]=  0; rgbw[3]=255; break;
          case 350: rgbw[0]=130; rgbw[1]= 90; rgbw[2]=  0; rgbw[3]=255; break;
          case 383: rgbw[0]=255; rgbw[1]=153; rgbw[2]=  0; rgbw[3]=255; break;
					default : colorKtoRGB(k, rgbw);
        }
      } else {
        colorKtoRGB(k, rgbw);
      }
      strip.setColor(0, rgbw[0], rgbw[1], rgbw[2], rgbw[3]);
    } else {
      uint32_t color = espalexaDevice->getRGB();
      strip.setColor(0, color);
    }
    stateUpdated(CALL_MODE_ALEXA);
  }
}


#else
 void alexaInit(){}
 void handleAlexa(){}
#endif
