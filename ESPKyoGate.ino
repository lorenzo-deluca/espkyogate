#include <EEPROM.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#include "./ESPKyoDef.h"

WiFiClient espClient;
PubSubClient client(espClient);

//#define LOG_SERIALE

//_GateConfig RuntimeConfig;
_GateConfig StartupConfig;

int FasePollingKyo;
int CmdExec;

long previousMillis = 0;      // Timer loop from http://www.arduino.cc/en/Tutorial/BlinkWithoutDelay
long interval = 1000;

void setup()
{
  // Imposto porta seriale a 9600, 8 bit dati, 1 bit stop e paritÃ  DISPARI
  Serial.begin(9600, SERIAL_8E1);

  // Carico configurazione da EEPROM (se c'Ã¨), altrimenti Default
  LoadConfigurazione();

  // Gestione WiFi
  setup_wifi();

  // comando asincrono da eseguire
  CmdExec = CMD_NO_CDM;

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length)
{
  String ComandoRicevuto = "";
  char Log[255];

  PrintDebug("Message arrived [");
  PrintDebug(topic);
  PrintDebug("] ");

  for (int i = 0; i < length; i++)
  {
    // Compongo Comando Ricevuto
    ComandoRicevuto = ComandoRicevuto + (char)payload[i];
  }

  PrintDebug(ComandoRicevuto);

  //sprintf(Log, "Ricevuto Comando <%s>", string2char(ComandoRicevuto));
  //client.publish(trace_topic, Log);

  GestioneComando(ComandoRicevuto);
}


void loop()
{
  char TraceString[255];

  // Serial.println("Ready");
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // millisecondi attuali
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval)
  {
      if (StartupConfig.PollingKyo)
      {
          if(KyoUnit_Polling())
          {
            // ad ogni polling aggiorno il keepalive dmy_PollingCentraleKyo (se non viene aggiornato per 30 secondi scatta l'allarme)
            if(StartupConfig.DomoticzUpdate && StartupConfig.DomoticzStateIdx > 0)
            {
                sprintf(TraceString, "{\"command\": \"setuservariable\", \"idx\": %d, \"value\": \"OK\"  }", StartupConfig.DomoticzStateIdx );
                client.publish(domoticz_topic, TraceString);
            }
          }
          
          FasePollingKyo = FASE_POLLING_KYO_STATO;
      }

      if(CmdExec != CMD_NO_CDM)
      {
          KyoUnit_GesCmdExec(CmdExec);
          CmdExec = CMD_NO_CDM;
      }

      // salvo ultima esecuzione
      previousMillis = currentMillis;
  }

}

