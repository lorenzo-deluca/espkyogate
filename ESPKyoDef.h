/*
  ESPKyoDef.h - ESPKyo32Gate

  Copyright (C) 2021 Lorenzo De Luca (me@lorenzodeluca.info)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#define mqtt_server "enter mqtt server"
#define mqtt_user ""
#define mqtt_password ""

#define wifi_ssid "enter wifi SSID"
#define wifi_password "enter wifi password"

#define trace_topic "ESPKyoGate/out"
#define command_topic "ESPKyoGate/in"
#define domoticz_topic "domoticz/in"

#define MAX_ZONE 32
#define MAX_AREE 8

typedef struct
{
    bool Abilitato;

    bool StatusAttuale;
    bool LastStatus;

    int DomoticzIdx;
} _Valore;

typedef struct
{
    // stato Zone
    struct Zona
    {
        bool Abilitata;
        int StatoAttivo;
        int DomoticzID;

        int LastStatus;
    } Zone[MAX_ZONE];

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
    } Warnings;

    bool PollingKyo = false;
    bool TracePolling = false;

    bool DomoticzUpdate = false;
    int DomoticzStateIdx = -1;

    char OkConfig[2];
} _GateConfig;

// Fasi Polling
#define FASE_POLLING_KYO_INIT 0
#define FASE_POLLING_KYO_STATO 1
#define FASE_POLLING_KYO_INSERIMENTI 2

#define FASE_POLLING_KYO_FINE 99

#define CMD_ARM_AWAY 1
#define CMD_ARM_HOME 2
#define CMD_DISARM 3
#define CMD_ACK_ALARM 4

#define CMD_NO_CDM -1
