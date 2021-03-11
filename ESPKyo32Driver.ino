/*
  ESPKyo32Driver.ino - ESPKyo32Gate

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

bool KyoUnit_Polling()
{
    while (FasePollingKyo != FASE_POLLING_KYO_FINE)
    {
        if (!Kyo32_SelPolling(FasePollingKyo))
            return false;

        delay(100);
    }

    // fine polling kyo
    delay(500);

    return true;
}

void KyoUnit_GesCmdExec(int Cmd)
{
    byte Rx[255];
    int Count = 0, i;

    char TraceString[255];
    byte message[255];

    switch (Cmd)
    {
    case CMD_ARM_AWAY:

        message[0] = 0x0F;
        message[1] = 0x00;
        message[2] = 0xF0;
        message[3] = 0x03;
        message[4] = 0x00;
        message[5] = 0x02;
        message[6] = 0x07;
        message[7] = 0x00;
        message[8] = 0x00;
        message[9] = 0x00;
        message[10] = 0x07;
        //byte message[] = {0x0F 0x00 0xF0 0x03 0x00 0x02 0x07 0x00 0x00 0x00 0x07};

        if (Kyo32_SendReceive(message, 11, Rx, Count))
        {
            sprintf(TraceString, "KyoUnit_GesCmdExec CMD_ARM_AWAY - %d Bytes", Count);
            client.publish(trace_topic, TraceString);
        }

        break;

    case CMD_ARM_HOME:

        message[0] = 0x0F;
        message[1] = 0x00;
        message[2] = 0xF0;
        message[3] = 0x03;
        message[4] = 0x00;
        message[5] = 0x02;
        message[6] = 0x04;
        message[7] = 0x00;
        message[8] = 0x00;
        message[9] = 0xFB;
        message[10] = 0xFF;
        //byte message[] = {0x0F 0x00 0xF0 0x03 0x00 0x02 0x04 0x00 0x00 0xFB 0xFF};

        if (Kyo32_SendReceive(message, 11, Rx, Count))
        {
            sprintf(TraceString, "KyoUnit_GesCmdExec CMD_ARM_HOME - %d Bytes", Count);
            client.publish(trace_topic, TraceString);
        }

        break;

    case CMD_DISARM:

        message[0] = 0x0F;
        message[1] = 0x00;
        message[2] = 0xF0;
        message[3] = 0x03;
        message[4] = 0x00;
        message[5] = 0x02;
        message[6] = 0x00;
        message[7] = 0x00;
        message[8] = 0x00;
        message[9] = 0x07;
        message[10] = 0x07;
        //byte message[] = {0x0F 0x00 0xF0 0x03 0x00 0x02 0x00 0x00 0x00 0x07 0x07};

        if (Kyo32_SendReceive(message, 11, Rx, Count))
        {
            sprintf(TraceString, "KyoUnit_GesCmdExec CMD_DISARM - %d Bytes", Count);
            client.publish(trace_topic, TraceString);
        }

        break;

    case CMD_ACK_ALARM:

        message[0] = 0x0F;
        message[1] = 0x05;
        message[2] = 0xF0;
        message[3] = 0x01;
        message[4] = 0x00;
        message[5] = 0x05;
        message[6] = 0x07;
        message[7] = 0x00;
        message[8] = 0x07;
        //byte message[] = {0F 05 F0 01 00 05 07 00 07};

        if (Kyo32_SendReceive(message, 8, Rx, Count))
        {
            sprintf(TraceString, "KyoUnit_GesCmdExec CMD_ACK_ALARM - %d Bytes", Count);
            client.publish(trace_topic, TraceString);
        }

        break;
    }
}

bool Kyo32_SelPolling(int FasePolling)
{
    byte Rx[255];
    int Count = 0, i;

    char TraceString[255];
    byte message[255];
    int StatoZona;
    byte mask = 1;

    memset(message, 0, sizeof(message));
    switch (FasePolling)
    {
    case FASE_POLLING_KYO_INIT:

        message[0] = 0xF0;
        message[1] = 0x00;
        message[2] = 0x00;
        message[3] = 0x0B;
        message[4] = 0x00;
        message[5] = 0xFB;
        //byte message[] = {0xF0, 0x00, 0x00, 0x0B, 0x00, 0xFB};

        if (Kyo32_SendReceive(message, 6, Rx, Count))
        {
            if (StartupConfig.TracePolling)
            {
                sprintf(TraceString, "FASE_POLLING_KYO_INIT - %d Bytes", Count);
                client.publish(trace_topic, TraceString);
            }
        }

        if (Count == 19)
        {
            FasePollingKyo = FASE_POLLING_KYO_STATO;
            return true;
        }
        else
            return false;

        break;

    case FASE_POLLING_KYO_INSERIMENTI:

        FasePollingKyo = FASE_POLLING_KYO_FINE;

        message[0] = 0xF0;
        message[1] = 0x02;
        message[2] = 0x15;
        message[3] = 0x12;
        message[4] = 0x00;
        message[5] = 0x19;
        //byte message[] = {0xF0 0x02 0x15 0x12 0x00 0x19};

        if (Kyo32_SendReceive(message, 6, Rx, Count))
        {
            if (StartupConfig.TracePolling)
            {
                sprintf(TraceString, "FASE_POLLING_KYO_INSERIMENTI - %d Bytes", Count);
                client.publish(trace_topic, TraceString);
            }
        }

        if (Count == 26)
        {
            if (StartupConfig.Warnings.StatoPiano.DomoticzIdx > 0)
            {
                StatoZona = (Rx[9] >> 0) & 1;

                if (StartupConfig.Warnings.StatoPiano.LastStatus != StatoZona)
                {
                    if (StartupConfig.Warnings.StatoPiano.DomoticzIdx != 0 && StartupConfig.DomoticzUpdate)
                    {
                        sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Warnings.StatoPiano.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                        client.publish(domoticz_topic, TraceString);
                    }

                    sprintf(TraceString, "Stato StatoPiano %d = %s", StartupConfig.Warnings.StatoPiano.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                    client.publish(trace_topic, TraceString);

                    StartupConfig.Warnings.StatoPiano.LastStatus = StatoZona;
                }
            }

            if (StartupConfig.Warnings.StatoMansarda.DomoticzIdx > 0)
            {
                StatoZona = (Rx[9] >> 1) & 1;

                if (StartupConfig.Warnings.StatoMansarda.LastStatus != StatoZona)
                {
                    if (StartupConfig.Warnings.StatoMansarda.DomoticzIdx != 0 && StartupConfig.DomoticzUpdate)
                    {
                        sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Warnings.StatoMansarda.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                        client.publish(domoticz_topic, TraceString);
                    }

                    sprintf(TraceString, "Stato StatoMansarda %d = %s", StartupConfig.Warnings.StatoMansarda.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                    client.publish(trace_topic, TraceString);

                    StartupConfig.Warnings.StatoMansarda.LastStatus = StatoZona;
                }
            }

            if (StartupConfig.Warnings.StatoPerimetro.DomoticzIdx > 0)
            {
                StatoZona = (Rx[9] >> 2) & 1;

                if (StartupConfig.Warnings.StatoPerimetro.LastStatus != StatoZona)
                {
                    if (StartupConfig.Warnings.StatoPerimetro.DomoticzIdx != 0 && StartupConfig.DomoticzUpdate)
                    {
                        sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Warnings.StatoPerimetro.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                        client.publish(domoticz_topic, TraceString);
                    }

                    sprintf(TraceString, "Stato StatoPerimetro %d = %s", StartupConfig.Warnings.StatoPerimetro.DomoticzIdx, (StatoZona == 1) ? "Off" : "On");
                    client.publish(trace_topic, TraceString);

                    StartupConfig.Warnings.StatoPerimetro.LastStatus = StatoZona;
                }
            }

            return true;
        }

        break;

    case FASE_POLLING_KYO_STATO:

        message[0] = 0xF0;
        message[1] = 0x04;
        message[2] = 0xF0;
        message[3] = 0x0A;
        message[4] = 0x00;
        message[5] = 0xEE;
        //byte message[] = {0xF0 0x04 0xF0 0x0A 0x00 0xEE};

        if (Kyo32_SendReceive(message, 6, Rx, Count))
        {
            if (StartupConfig.TracePolling)
            {
                sprintf(TraceString, "FASE_POLLING_KYO_STATO - %d Bytes", Count);
                client.publish(trace_topic, TraceString);
            }
        }

        FasePollingKyo = FASE_POLLING_KYO_INSERIMENTI;

        int MaxZone = 0;
        switch(Count)
        {
            // Kyo32G
            case 18:
                MaxZone = MAX_ZONE;
            break;

            // Kyo8
            case 12:
                MaxZone = ;
            break;

            default:
                return false;
        }
       
        // Ciclo ZONE
        for (i = 0; i < MaxZone; i++)
        {
            mask = 00000001;
            StatoZona = -1;

            if (StartupConfig.Zone[i].Abilitata)
            {
                if (MaxZone == MAX_ZONE_KYO_8)
                {
                    StatoZona = (Rx[6] >> i) & 1;
                }
                else
                {
                    if (i >= 24)
                        StatoZona = (Rx[6] >> (i - 24)) & 1;
                    else if (i >= 16 && i <= 23)
                        StatoZona = (Rx[7] >> (i - 16)) & 1;
                    else if (i >= 8 && i <= 15)
                        StatoZona = (Rx[8] >> (i - 8)) & 1;
                    else if (i <= 7)
                        StatoZona = (Rx[9] >> i) & 1;
                }

                if (StartupConfig.Zone[i].LastStatus != StatoZona)
                {
                    if (StartupConfig.Zone[i].DomoticzID != 0 && StartupConfig.DomoticzUpdate)
                    {
                        sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Zone[i].DomoticzID, (StatoZona == StartupConfig.Zone[i].StatoAttivo) ? "On" : "Off");
                        client.publish(domoticz_topic, TraceString);
                    }

                    sprintf(TraceString, "Stato Zona %d = %s", StartupConfig.Zone[i].DomoticzID, (StatoZona == StartupConfig.Zone[i].StatoAttivo) ? "On" : "Off");
                    client.publish(trace_topic, TraceString);

                    StartupConfig.Zone[i].LastStatus = StatoZona;
                }
            }
        }

        // Ciclo AREE
        for (i = 0; i < MAX_AREE; i++)
        {
            StatoZona = -1;

            if (StartupConfig.Area[i].Abilitato)
            {
                if (MaxZone == MAX_ZONE_KYO_8)
                    StatoZona = (Rx[9] >> i) & 1;
                else
                    StatoZona = (Rx[15] >> i) & 1;

                if (StartupConfig.Area[i].LastStatus != StatoZona)
                {
                    if (StartupConfig.Area[i].DomoticzIdx != 0 && StartupConfig.DomoticzUpdate)
                    {
                        sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Area[i].DomoticzIdx, (StatoZona == 1) ? "On" : "Off");
                        client.publish(domoticz_topic, TraceString);
                    }

                    sprintf(TraceString, "Stato Area %d = %s", StartupConfig.Area[i].DomoticzIdx, (StatoZona == 1) ? "On" : "Off");
                    client.publish(trace_topic, TraceString);

                    StartupConfig.Area[i].LastStatus = StatoZona;
                }
            }
        }

        // Warnings
        if (StartupConfig.Warnings.MancanzaRete.DomoticzIdx > 0)
        {
            if (MaxZone == MAX_ZONE_KYO_8)
                StatoZona = (Rx[8] >> 0) & 1;
            else
                StatoZona = (Rx[14] >> 0) & 1;

            if (StartupConfig.Warnings.MancanzaRete.LastStatus != StatoZona)
            {
                if (StartupConfig.Warnings.MancanzaRete.DomoticzIdx != 0 && StartupConfig.DomoticzUpdate)
                {
                    sprintf(TraceString, "{\"command\": \"switchlight\", \"idx\": %d, \"switchcmd\": \"%s\"  }", StartupConfig.Warnings.MancanzaRete.DomoticzIdx, (StatoZona == 1) ? "On" : "Off");
                    client.publish(domoticz_topic, TraceString);
                }

                sprintf(TraceString, "Stato MancanzaRete %d = %s", StartupConfig.Warnings.MancanzaRete.DomoticzIdx, (StatoZona == 1) ? "On" : "Off");
                client.publish(trace_topic, TraceString);

                StartupConfig.Warnings.MancanzaRete.LastStatus = StatoZona;
            }
        }

        return true;
        break;
    }

    return false;
}

bool Kyo32_SendReceive(byte message[], int size, byte ReadByes[], int &CountBytes)
{
    byte RxBuff[255];
    char TraceString[255];

    int index = 0, i = 0;

    Serial.flush(); // prova

    memset(ReadByes, 0, CountBytes);

    Serial.write(message, size);

    if (StartupConfig.TracePolling)
    {
        memset(TraceString, 0, 255);

        sprintf(TraceString, "Tx <%d> = ", size);
        for (i = 0; i < size; i++)
        {
            sprintf(TraceString, "%s %02X", TraceString, message[i]);
        }

        client.publish(trace_topic, TraceString);
    }

    // attendo risposta
    delay(100); //era 50

    // leggo risposta
    while (Serial.available() > 0)
    {
        RxBuff[index++] = Serial.read(); // Read a Byte
        delay(3);
    }

    if (StartupConfig.TracePolling)
    {
        memset(TraceString, 0, 255);
        sprintf(TraceString, "Rx <%d> =", index);

        for (i = 0; i < index; i++)
        {
            sprintf(TraceString, "%s %02X", TraceString, RxBuff[i]);
        }

        client.publish(trace_topic, TraceString);
    }

    if (index > 0)
    {
        CountBytes = index;
        memcpy(ReadByes, RxBuff, CountBytes);

        return true;
    }
    else
        return false;
}
