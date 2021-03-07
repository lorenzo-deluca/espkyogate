void storeStruct(void *data_source, size_t size)
{
  EEPROM.begin(size * 2);
  for(size_t i = 0; i < size; i++)
  {
    char data = ((char *)data_source)[i];
    EEPROM.write(i, data);
  }
  EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size)
{
    EEPROM.begin(size * 2);
    for(size_t i = 0; i < size; i++)
    {
        char data = EEPROM.read(i);
        ((char *)data_dest)[i] = data;
    }
}

void PrintDebug(String str)
{
#ifdef LOG_SERIALE
  Serial.println(str);
#endif

  return;
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") + \
         String(ipAddress[1]) + String(".") + \
         String(ipAddress[2]) + String(".") + \
         String(ipAddress[3])  ;
}

char* string2char(String command)
{
  if (command.length() != 0) {
    char *p = const_cast<char*>(command.c_str());
    return p;
  }
}

void setup_wifi()
{
  delay(10);

  PrintDebug("");
  PrintDebug("Connecting to ");
  PrintDebug(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    PrintDebug(".");
  }

  randomSeed(micros());

  PrintDebug("");
  PrintDebug("WiFi connected");
  PrintDebug("IP address: ");
  PrintDebug(IpAddress2String(WiFi.localIP()));
}

bool DefaultConfig()
{
    int i;
    
    for (i = 0; i < MAX_ZONE; i++)
    {
        StartupConfig.Zone[i].Abilitata = false;

        StartupConfig.Zone[i].StatoAttivo = -1;
        StartupConfig.Zone[i].DomoticzID = -1;     
        StartupConfig.Zone[i].LastStatus = -1;
    }

    _Valore defValue;
    defValue.Abilitato      = false;
    defValue.LastStatus     = -1;
    defValue.StatusAttuale  = -1;
    defValue.DomoticzIdx    = -1;

    for (i = 0; i < MAX_AREE; i++)
        StartupConfig.Area[i] = defValue;
    
    // resetto stato warnings
    StartupConfig.Warnings.MancanzaRete = defValue;
    StartupConfig.Warnings.StatoPiano = defValue;
    StartupConfig.Warnings.StatoMansarda = defValue;
    StartupConfig.Warnings.StatoPerimetro = defValue;
    
    /*StartupConfig.Warnings.WarningFusibile = defValue;
    StartupConfig.Warnings.BatteriaBassa = defValue;
    StartupConfig.Warnings.GuastoTelefono = defValue;
    StartupConfig.Warnings.WarningWireless = defValue;*/

    StartupConfig.PollingKyo = false;
    StartupConfig.TracePolling = false;
    
    StartupConfig.DomoticzUpdate = false;
    StartupConfig.DomoticzStateIdx = -1;

    memset(StartupConfig.OkConfig, 0, sizeof(StartupConfig.OkConfig));

    //memcpy(&RuntimeConfig, &StartupConfig,  sizeof(RuntimeConfig));
    
    return true;
}

bool LoadConfigurazione()
{
    int i;
    
    _GateConfig settings;

    // Carico nella struttura temporanea
    loadStruct(&settings, sizeof(StartupConfig));
    
    //EEPROM.get(0, StartupConfig);

    // Carico Configurazione (se c'Ã¨)
    if(settings.OkConfig[0] == 'O' && settings.OkConfig[1] == 'K')
    {
        PrintDebug("Carico Configurazione Salvata");

        memcpy(&StartupConfig, &settings, sizeof(StartupConfig));
        
        // resetto ultimo status (su nuova accensione aggiorno stato zone)
        for (i = 0; i < MAX_ZONE; i++)
          StartupConfig.Zone[i].LastStatus = -1;

        for(i = 0; i < MAX_AREE; i++)
           StartupConfig.Area[i].LastStatus = -1;

        StartupConfig.Warnings.MancanzaRete.LastStatus = -1;
/*        StartupConfig.Warnings.WarningFusibile.LastStatus = -1;
        StartupConfig.Warnings.BatteriaBassa.LastStatus = -1;
        StartupConfig.Warnings.GuastoTelefono.LastStatus = -1;
        StartupConfig.Warnings.WarningWireless.LastStatus = -1;
*/        
    }
    else
    {
        PrintDebug("Nessuna Configurazione Salvata - Carico Default");
        return DefaultConfig();
    }

    return true;
}

bool SalvaConfigurazione()
{
    StartupConfig.OkConfig[0] = 'O';
    StartupConfig.OkConfig[1] = 'K';

    storeStruct(&StartupConfig, sizeof(StartupConfig));
  
    //EEPROM.put(0, StartupConfig);
    //EEPROM.commit();
    
    return true;
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    PrintDebug("Attempting MQTT connection...");

    // Provo a connettermi al server MQTT
    if (client.connect("ESPKyoGate", mqtt_user, mqtt_password)) {
      PrintDebug("connected");
      client.publish(trace_topic, "ESP Connesso ");

      //      PrintConfigurazione(true);
      //      PrintConfigurazione(false);

      // Sottoscrivo canale ricezione comandi
      client.subscribe(command_topic);

    } else {
      PrintDebug("failed, rc=");
      PrintDebug(String(client.state()));
      PrintDebug(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


