/*
ESP8266_ArtNetNode v2.0.0
Copyright (c) 2016, Matthew Tong
https://github.com/mtongnz/ESP8266_ArtNetNode_v2

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.
If not, see http://www.gnu.org/licenses/
*/


void doNodeReport() {
	if (nextNodeReport > millis())
		return;

	char c[ARTNET_NODE_REPORT_LENGTH];

	if (nodeErrorTimeout > millis())
		nextNodeReport = millis() + 2000;
	else
		nextNodeReport = millis() + 5000;

	if (nodeError[0] != '\0' && !nodeErrorShowing && nodeErrorTimeout > millis()) {

		nodeErrorShowing = true;
		strcpy(c, nodeError);

	}
	else {
		nodeErrorShowing = false;

		strcpy(c, "OK: PortA:");

		switch (deviceSettings.portAmode) {
		case TYPE_DMX_OUT:
			sprintf(c, "%s DMX Out", c);
			break;

		case TYPE_RDM_OUT:
			sprintf(c, "%s RDM Out", c);
			break;

		case TYPE_DMX_IN:
			sprintf(c, "%s DMX In", c);
			break;

		case TYPE_WS2812:
			if (deviceSettings.portApixMode == FX_MODE_12)
				sprintf(c, "%s 12chan", c);
			sprintf(c, "%s WS2812 %ipixels", c, deviceSettings.portAnumPix);
			break;
		}

		sprintf(c, "%s. PortB:", c);

		switch (deviceSettings.portBmode) {
		case TYPE_DMX_OUT:
			sprintf(c, "%s DMX Out", c);
			break;

		case TYPE_RDM_OUT:
			sprintf(c, "%s RDM Out", c);
			break;

		case TYPE_WS2812:
			if (deviceSettings.portBpixMode == FX_MODE_12)
				sprintf(c, "%s 12chan", c);
			sprintf(c, "%s WS2812 %ipixels", c, deviceSettings.portBnumPix);
			break;
		}
	}

	artRDM.setNodeReport(c, ARTNET_RC_POWER_OK);
}

void portSetup()
{
	// PORT A INITIALIZATION
#ifndef DEBUG_ENABLE  // if debug is enabled the port A is not used
	if (deviceSettings.portAmode == TYPE_DMX_OUT || deviceSettings.portAmode == TYPE_RDM_OUT)
	{
		LogLn("Port A configured as Output for universe "); Log((String)portA[0]); Log(" in subnet "); Log((String)portA[1]); LogLn(".");
		delay(100); // wait until logged

		setDmxLed(DMX_ACT_LED_A, true);      // led bright when output

		dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
		if (deviceSettings.portAmode == TYPE_RDM_OUT && !dmxA.rdmEnabled())
		{
			dmxA.rdmEnable(ESTA_MAN, ESTA_DEV);
			dmxA.rdmSetCallBack(rdmReceivedA); // RDM Callback handler :> rdmReceivedA
			dmxA.todSetCallBack(sendTodA);     // ToD = Table of Devices
		}

	}
	else if (deviceSettings.portAmode == TYPE_DMX_IN)
	{
		LogLn("Port A configured as Input.");
		setDmxLed(DMX_ACT_LED_A, false);    // led dark when input

		dmxA.begin(DMX_DIR_A, artRDM.getDMX(portA[0], portA[1]));
		dmxA.dmxIn(true);
		dmxA.setInputCallback(dmxIn);

		dataIn = (byte*)os_malloc(sizeof(byte) * 512);   // allocate storage
		memset(dataIn, 0, 512);
	}
	else if (deviceSettings.portAmode == TYPE_WS2812)
	{
		LogLn("Port A configured as WS2812.");
		setDmxLed(DMX_ACT_LED_A, true);     // led bright when TYPE_WS2812

		digitalWrite(DMX_DIR_A, HIGH);      // manually set to high
		pixDriver.setStrip(0, DMX_TX_A, deviceSettings.portAnumPix, deviceSettings.portApixConfig);
	}
#endif

	// PORT B INITIALIZATION
	if (deviceSettings.portBmode == TYPE_DMX_OUT || deviceSettings.portBmode == TYPE_RDM_OUT)
	{
		Log("Port B configured as Output for universe "); Log((String)portB[0]); Log(" in subnet "); Log((String)portB[1]); LogLn(".");
		delay(100); // wait until logged

		setDmxLed(DMX_ACT_LED_B, true);     // led bright when output

		dmxB.begin(DMX_DIR_B, artRDM.getDMX(portB[0], portB[1]));
		if (deviceSettings.portBmode == TYPE_RDM_OUT && !dmxB.rdmEnabled()) {
			dmxB.rdmEnable(ESTA_MAN, ESTA_DEV);
			dmxB.rdmSetCallBack(rdmReceivedB);
			dmxB.todSetCallBack(sendTodB);
		}
	}
	/// Port B cannot be configured as Input (only 1 Input allowed)
	else if (deviceSettings.portBmode == TYPE_WS2812)
	{
		LogLn("Port B configured as WS2812.");
		setDmxLed(DMX_ACT_LED_B, true);     // led bright when TYPE_WS2812

		digitalWrite(DMX_DIR_B, HIGH);
		pixDriver.setStrip(1, DMX_TX_B, deviceSettings.portBnumPix, deviceSettings.portBpixConfig);
	}
	else
	{
		setDmxLed(DMX_ACT_LED_B, false);     // led bright when other
	}

	pixDriver.allowInterruptSingle = WS2812_ALLOW_INT_SINGLE;
	pixDriver.allowInterruptDouble = WS2812_ALLOW_INT_DOUBLE;
}

void artStart()
{
	LogLn("Starting ArtNet/sACN...");

	// Initialise out ArtNet
	if (isHotspot)
		artRDM.init(deviceSettings.hotspotIp, deviceSettings.hotspotSubnet, true, deviceSettings.nodeName, deviceSettings.longName, ARTNET_OEM, ESTA_MAN, macAddress);
	else
		artRDM.init(deviceSettings.ip, deviceSettings.subnet, deviceSettings.dhcpEnable, deviceSettings.nodeName, deviceSettings.longName, ARTNET_OEM, ESTA_MAN, macAddress);

	// Set firmware
	artRDM.setFirmwareVersion(ART_FIRM_VERSION);

	// Add Group
	portA[0] = artRDM.addGroup(deviceSettings.portAnet, deviceSettings.portAsub);

	// copy the protocol type from the storage into an enum
	p_protocol protocolTypeA = (p_protocol)deviceSettings.portAprot;

  if(protocolTypeA == PROT_ARTNET)  // only if artnet
  {
  	// WS2812 uses TYPE_DMX_OUT - the rest use the value assigned
  	if (deviceSettings.portAmode == TYPE_WS2812)
  		portA[1] = artRDM.addPort(portA[0], 0, deviceSettings.portAuni[0], TYPE_DMX_OUT, deviceSettings.portAmerge);
  	else
  		portA[1] = artRDM.addPort(portA[0], 0, deviceSettings.portAuni[0], deviceSettings.portAmode, deviceSettings.portAmerge);
  }

	// if it is an sACN type
	else
		startE131(e131A, protocolTypeA, deviceSettings.portAuni[0]);
 
	// Add extra Artnet ports for WS2812
	//if (deviceSettings.portAmode == TYPE_WS2812 && deviceSettings.portApixMode == FX_MODE_PIXEL_MAP)
	//{
	//	if (deviceSettings.portAnumPix > 170)
	//	{
	//		portA[2] = artRDM.addPort(portA[0], 1, deviceSettings.portAuni[1], TYPE_DMX_OUT, deviceSettings.portAmerge);

	//		artRDM.setProtocolType(portA[0], portA[2], protocolTypeA);
	//		artRDM.setE131Uni(portA[0], portA[2], deviceSettings.portAsACNuni[1]);
	//	}
	//	if (deviceSettings.portAnumPix > 340)
	//	{
	//		portA[3] = artRDM.addPort(portA[0], 2, deviceSettings.portAuni[2], TYPE_DMX_OUT, deviceSettings.portAmerge);

	//		artRDM.setProtocolType(portA[0], portA[3], protocolTypeA);
	//		artRDM.setE131Uni(portA[0], portA[3], deviceSettings.portAsACNuni[2]);
	//	}
	//	if (deviceSettings.portAnumPix > 510)
	//	{
	//		portA[4] = artRDM.addPort(portA[0], 3, deviceSettings.portAuni[3], TYPE_DMX_OUT, deviceSettings.portAmerge);

	//		artRDM.setProtocolType(portA[0], portA[4], protocolTypeA);
	//		artRDM.setE131Uni(portA[0], portA[4], deviceSettings.portAsACNuni[3]);
	//	}
	//}


	// Add Group
	portB[0] = artRDM.addGroup(deviceSettings.portBnet, deviceSettings.portBsub);


	// copy the protocol type from the storage into an enum
	p_protocol protocolTypeB = (p_protocol)deviceSettings.portBprot;

  if(protocolTypeB == PROT_ARTNET)  // only if artnet
  {
  	// WS2812 uses TYPE_DMX_OUT - the rest use the value assigned
  	if (deviceSettings.portBmode == TYPE_WS2812)
  		portB[1] = artRDM.addPort(portB[0], 0, deviceSettings.portBuni[0], TYPE_DMX_OUT, deviceSettings.portBmerge);
  	else
  		portB[1] = artRDM.addPort(portB[0], 0, deviceSettings.portBuni[0], deviceSettings.portBmode, deviceSettings.portBmerge);
  }
  
	// if it is an sACN type
	else
		startE131(e131B, protocolTypeB, deviceSettings.portBuni[0]);
	

	// Add extra Artnet ports for WS2812
	//if (deviceSettings.portBmode == TYPE_WS2812 && deviceSettings.portBpixMode == FX_MODE_PIXEL_MAP)
	//{
	//	if (deviceSettings.portBnumPix > 170)
	//	{
	//		portB[2] = artRDM.addPort(portB[0], 1, deviceSettings.portBuni[1], TYPE_DMX_OUT, deviceSettings.portBmerge);

	//		artRDM.setProtocolType(portB[0], portB[2], protocolTypeB);
	//		artRDM.setE131Uni(portB[0], portB[2], deviceSettings.portBsACNuni[1]);
	//	}
	//	if (deviceSettings.portBnumPix > 340)
	//	{
	//		portB[3] = artRDM.addPort(portB[0], 2, deviceSettings.portBuni[2], TYPE_DMX_OUT, deviceSettings.portBmerge);

	//		artRDM.setProtocolType(portB[0], portB[3], protocolTypeB);
	//		artRDM.setE131Uni(portB[0], portB[3], deviceSettings.portBsACNuni[2]);
	//	}
	//	if (deviceSettings.portBnumPix > 510)
	//	{
	//		portB[4] = artRDM.addPort(portB[0], 3, deviceSettings.portBuni[3], TYPE_DMX_OUT, deviceSettings.portBmerge);

	//		artRDM.setProtocolType(portB[0], portB[4], protocolTypeB);
	//		artRDM.setE131Uni(portB[0], portB[4], deviceSettings.portBsACNuni[3]);
	//	}
	//}

	// Add required callback functions
	artRDM.setArtDMXCallback(dmxHandle);
	artRDM.setArtRDMCallback(rdmHandle);
	artRDM.setArtSyncCallback(syncHandle);
	artRDM.setArtIPCallback(ipHandle);
	artRDM.setArtAddressCallback(addressHandle);
	artRDM.setTODRequestCallback(todRequest);
	artRDM.setTODFlushCallback(todFlush);


	switch (resetInfo.reason) {
	case REASON_DEFAULT_RST:  // normal start
	case REASON_EXT_SYS_RST:
	case REASON_SOFT_RESTART:
		artRDM.setNodeReport("OK: Device started", ARTNET_RC_POWER_OK);
		nextNodeReport = millis() + 4000;
		break;

	case REASON_WDT_RST:
		artRDM.setNodeReport("ERROR: (HWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
		strcpy(nodeError, "Restart error: HWDT");
		nextNodeReport = millis() + 10000;
		nodeErrorTimeout = millis() + 30000;
		break;

	case REASON_EXCEPTION_RST:
		artRDM.setNodeReport("ERROR: (EXCP) Unexpected device restart", ARTNET_RC_POWER_FAIL);
		strcpy(nodeError, "Restart error: EXCP");
		nextNodeReport = millis() + 10000;
		nodeErrorTimeout = millis() + 30000;
		break;

	case REASON_SOFT_WDT_RST:
		artRDM.setNodeReport("ERROR: (SWDT) Unexpected device restart", ARTNET_RC_POWER_FAIL);
		strcpy(nodeError, "Error on Restart: SWDT");
		nextNodeReport = millis() + 10000;
		nodeErrorTimeout = millis() + 30000;
		break;

	case REASON_DEEP_SLEEP_AWAKE:
		// not used
		break;
	}

	// Start artnet
	artRDM.begin();
	LogLn("ArtNet/sACN started");

	yield();
}

void startE131(E131 e131, p_protocol type, uint8_t universe)
{
	e131_listen_t listenType = E131_MULTICAST;  // default multicast

	if (type == PROT_SACN_UNICAST)  // if unicast => switch listenType
		listenType = E131_UNICAST;

	// begin listening => only if the universe is not already correctly set
	if (e131.universe != universe)
		e131.begin(listenType, universe, 1);
}

void webStart()
{
	LogLn("Starting Webserver...");

	webServer.on("/", []()
	{
		artRDM.pause();
		webServer.send_P(200, typeHTML, mainPage); //Send web page
		webServer.sendHeader("Connection", "close");
		yield();
		artRDM.begin();
	});

	webServer.on("/style.css", []()
	{
		artRDM.pause();

		File f = SPIFFS.open("/style.css", "r");

		// If no style.css in SPIFFS, send default
		if (!f)
			webServer.send_P(200, typeCSS, css);
		else
			size_t sent = webServer.streamFile(f, typeCSS);

		f.close();
		webServer.sendHeader("Connection", "close");

		yield();
		artRDM.begin();
	});

	webServer.on("/ajax", HTTP_POST, ajaxHandle);

	webServer.on("/upload", HTTP_POST, webFirmwareUpdate, webFirmwareUpload);

	webServer.on("/style", []()
	{
		webServer.send_P(200, typeHTML, cssUploadPage);
		webServer.sendHeader("Connection", "close");
	});

	webServer.on("/style_delete", []()
	{
		if (SPIFFS.exists("/style.css"))
			SPIFFS.remove("/style.css");

		webServer.send(200, "text/plain", "style.css deleted.  The default style is now in use.");
		webServer.sendHeader("Connection", "close");
	});

	webServer.on("/style_upload", HTTP_POST, []()
	{
		webServer.send(200, "text/plain", "Upload successful!");
	}, []() {
		HTTPUpload& upload = webServer.upload();

		if (upload.status == UPLOAD_FILE_START) {
			String filename = upload.filename;
			if (!filename.startsWith("/")) filename = "/" + filename;
			fsUploadFile = SPIFFS.open(filename, "w");
			filename = String();

		}
		else if (upload.status == UPLOAD_FILE_WRITE) {
			if (fsUploadFile)
				fsUploadFile.write(upload.buf, upload.currentSize);

		}
		else if (upload.status == UPLOAD_FILE_END) {
			if (fsUploadFile) {
				fsUploadFile.close();

				if (upload.filename != "/style.css")
					SPIFFS.rename(upload.filename, "/style.css");
			}
		}
	});

	webServer.onNotFound([]()
	{
		webServer.send(404, "text/plain", "Page not found");
	});

	// Start webserver
	webServer.begin();

	LogLn("Webserver started");
	yield();
}

void wifiStart()
{
	// If it's the default WiFi SSID, make it unique
	if (strcmp(deviceSettings.hotspotSSID, "espArtNetNode") == 0 || deviceSettings.hotspotSSID[0] == '\0')
		sprintf(deviceSettings.hotspotSSID, "espArtNetNode_%05u", (ESP.getChipId() & 0xFF));

	if (deviceSettings.standAloneEnable)
	{
		LogLn("Standalone is enabled");
		startHotspot();

		deviceSettings.ip = deviceSettings.hotspotIp;
		deviceSettings.subnet = deviceSettings.hotspotSubnet;
		deviceSettings.broadcast = { ~deviceSettings.subnet[0] | (deviceSettings.ip[0] & deviceSettings.subnet[0]), ~deviceSettings.subnet[1] | (deviceSettings.ip[1] & deviceSettings.subnet[1]), ~deviceSettings.subnet[2] | (deviceSettings.ip[2] & deviceSettings.subnet[2]), ~deviceSettings.subnet[3] | (deviceSettings.ip[3] & deviceSettings.subnet[3]) };

		return;
	}

	if (deviceSettings.wifiSSID[0] != '\0')
	{
		Log("Connecting to WiFi "); Log(deviceSettings.wifiSSID); Log(" as "); Log(deviceSettings.nodeName);
		// Bring up the WiFi connection
		WiFi.mode(WIFI_STA);
		WiFi.persistent(false);		//prevent excesive writing to flash
		WiFi.setAutoConnect(true);
		WiFi.setAutoReconnect(true);
		WiFi.hostname(deviceSettings.nodeName);
		WiFi.begin(deviceSettings.wifiSSID, deviceSettings.wifiPass);

		unsigned long endTime = millis() + (deviceSettings.hotspotDelay * 1000);

		while (WiFi.status() != WL_CONNECTED) {
			// Check to see if credentials are wrong
			if (WiFi.status() == WL_CONNECT_FAILED) {
				LogLn("Failed to connect to WiFi. Please verify credentials!");
				delay(1000);
				startHotspot();
			}
			delay(100);
			Log(".");
			delay(500);
			// Only try for 5 seconds.
			if (millis() >= endTime) {
				LogLn(".");
				Log("Failed to connect to WiFi");
				startHotspot();
				return;
			}
		}

		if (deviceSettings.dhcpEnable)
		{
			deviceSettings.ip = WiFi.localIP();
			deviceSettings.subnet = WiFi.subnetMask();

			if (deviceSettings.gateway == INADDR_NONE)
				deviceSettings.gateway = WiFi.gatewayIP();

			deviceSettings.broadcast = { ~deviceSettings.subnet[0] | (deviceSettings.ip[0] & deviceSettings.subnet[0]), ~deviceSettings.subnet[1] | (deviceSettings.ip[1] & deviceSettings.subnet[1]), ~deviceSettings.subnet[2] | (deviceSettings.ip[2] & deviceSettings.subnet[2]), ~deviceSettings.subnet[3] | (deviceSettings.ip[3] & deviceSettings.subnet[3]) };
		}
		else
			WiFi.config(deviceSettings.ip, deviceSettings.gateway, deviceSettings.subnet);

		sprintf(wifiStatus, "Wifi connected.<br />SSID: %s", deviceSettings.wifiSSID);
		LogLn("WiFi Connection Success.");
		setStatusLed(GREEN);    //Green when connected to Wlan
		WiFi.macAddress(macAddress);

	}
	else
		startHotspot();

	yield();
}

void startHotspot()
{
	yield();

	Log("Starting Hotspot with SSID "); Log(deviceSettings.hotspotSSID); Log(" IP: "); Log(deviceSettings.hotspotIp.toString()); LogLn(".");
	WiFi.mode(WIFI_AP);
	WiFi.softAP(deviceSettings.hotspotSSID, deviceSettings.hotspotPass);
	WiFi.softAPConfig(deviceSettings.hotspotIp, deviceSettings.hotspotIp, deviceSettings.hotspotSubnet);

	sprintf(wifiStatus, "No Wifi. Hotspot started.<br />\nHotspot SSID: %s", deviceSettings.hotspotSSID);
	setStatusLed(ORANGE);   //Orange when in Hotspot Mode
	WiFi.macAddress(macAddress);

	isHotspot = true;

	if (deviceSettings.standAloneEnable)
		return;

	webStart();

	unsigned long endTime = millis() + 30000;

	// Stay here if not in stand alone mode - no dmx or artnet
	while (endTime > millis() || wifi_softap_get_station_num() > 0) {
		webServer.handleClient();
		yield();
	}

	ESP.restart();
	isHotspot = false;
}
