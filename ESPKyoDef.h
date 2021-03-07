
#define mqtt_server "enter mqtt server"
#define mqtt_user ""
#define mqtt_password ""

#define wifi_ssid "enter wifi SSID"
#define wifi_password "enter wifi password"

#define trace_topic "ESPKyoGate/out"
#define command_topic "ESPKyoGate/in"
#define domoticz_topic "domoticz/in"

#define MAX_ZONE  32
#define MAX_AREE  8

typedef struct 
{
    bool Abilitato;
    
    bool StatusAttuale;
    bool LastStatus;

    int DomoticzIdx;
}_Valore;

typedef struct 
{
   // stato Zone
   struct Zona
   {
      bool Abilitata;
      int StatoAttivo;
      int DomoticzID;
      
      int LastStatus;
   }Zone[MAX_ZONE];

    // stato Aree
    _Valore Area[MAX_AREE];
    
    // Stato Warnings
    struct 
    {
        _Valore MancanzaRete;
        _Valore StatoPiano;
        _Valore StatoMansarda;
        _Valore StatoPerimetro;
        _Valore WarningWireless;
    }
    Warnings; 

    bool PollingKyo = false;
    bool TracePolling = false;

    bool DomoticzUpdate = false;
    int DomoticzStateIdx = -1;

    char OkConfig[2];
}_GateConfig;


// Fasi Polling
#define FASE_POLLING_KYO_INIT           0
#define FASE_POLLING_KYO_STATO          1
#define FASE_POLLING_KYO_INSERIMENTI    2

#define FASE_POLLING_KYO_FINE           99

#define CMD_ARM_AWAY                    1
#define CMD_ARM_HOME                    2
#define CMD_DISARM                      3
#define CMD_ACK_ALARM                   4

#define CMD_NO_CDM                      -1
