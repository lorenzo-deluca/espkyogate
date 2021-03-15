/*
  ESPKyo_GesComandi.ino - ESPKyo32Gate

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

void GestioneComando(String ComandoRicevuto)
{
    // Gestisco comando ricevuto
    // la codifica del comando è delimitata da " "
    String Comando = "";
    String SecondoComando = "";

    String arg1 = "";
    String arg2 = "";
    String arg3 = "";
    String arg4 = "";

    char TraceString[255];
    int CodiceZona = -1;
    int CodiceArea = -1;

    Comando = getValue(ComandoRicevuto, ' ', 0);

    //
    // Comando "?"
    //
    if (Comando == String("?"))
    {
        //client.publish(trace_topic, "<?>=Help\n<show> [run/start]=Runtime/Startup Config\n");
        client.publish(trace_topic, "<?> Help\n<start/stop>\n<trace/notrace>\n<conf> Config \n<show> Show Config\nCmdExec <armhome/armaway/disarm>\n<restart/reset>\n<save>\n<default>");
    }
    else if (Comando == String("CmdExec"))
    {
        // CmdExec - primo comando
        //    <armhome> <armaway> <disarm> <ack> - secondo comando
        sprintf(TraceString, "Eseguo CmdExec!");

        SecondoComando = getValue(ComandoRicevuto, ' ', 1); // SecondoComando

        if (SecondoComando == "armhome")
        {
            CmdExec = CMD_ARM_HOME;
        }
        else if (SecondoComando == "armaway")
        {
            CmdExec = CMD_ARM_AWAY;
        }
        else if (SecondoComando == "disarm")
        {
            CmdExec = CMD_DISARM;
        }
        else if (SecondoComando == "ack")
        {
            CmdExec = CMD_ACK_ALARM;
        }
    }
    //
    // Comando "conf"
    //
    else if (Comando == String("conf"))
    {
        // CONF - primo comando
        //    ZONE <NumeroZona> <Abilitata> <StatoAttivo> <DomoticzID> - secondo comando
        //    PARAM <PARAMETRO> <VALORE>                               - terzo comando
        sprintf(TraceString, "Errore Configurazione!");

        SecondoComando = getValue(ComandoRicevuto, ' ', 1); // SecondoComando

        if (SecondoComando == String("?"))
        {
            sprintf(TraceString, "conf zone <Zona> <Abil> <StatoAtt> <Idx>\nconf area <Area> <Abil> <Idx>\n\nconf param <?/PARAMETRO> <VALORE>");
        }
        else if (SecondoComando == String("zone"))
        {
            // non si può effettuare la configurazione con polling attivo
            if (!StartupConfig.PollingKyo)
            {
                arg1 = getValue(ComandoRicevuto, ' ', 2); // NumeroZona
                arg2 = getValue(ComandoRicevuto, ' ', 3); // Abilitata
                arg3 = getValue(ComandoRicevuto, ' ', 4); // Stato Attivo
                arg4 = getValue(ComandoRicevuto, ' ', 5); // DomoticzID

                CodiceZona = arg1.toInt() - 1;

                if (CodiceZona >= 0 && CodiceZona < MAX_ZONE)
                {
                    StartupConfig.Zone[CodiceZona].Abilitata = (arg2.toInt() == 1);
                    StartupConfig.Zone[CodiceZona].StatoAttivo = (arg3.toInt() == 1);
                    StartupConfig.Zone[CodiceZona].DomoticzID = arg4.toInt();

                    sprintf(TraceString, "Modificata Zona <%d> - Abilitata <%d> - Stato Attivo <%d> - DomoticzID <%d>",
                            CodiceZona + 1,
                            (arg2.toInt() == 1),
                            (arg3.toInt() == 1),
                            arg4.toInt());
                }
                else
                {
                    sprintf(TraceString, "Errore CodiceZona <%d>", CodiceZona);
                }
            }
            else
            {
                sprintf(TraceString, "Errore Polling Attivo!");
            }
        }
        else if (SecondoComando == String("area"))
        {
            // non si può effettuare la configurazione con polling attivo
            if (!StartupConfig.PollingKyo)
            {
                arg1 = getValue(ComandoRicevuto, ' ', 2); // NumeroZona
                arg2 = getValue(ComandoRicevuto, ' ', 3); // Abilitata
                arg3 = getValue(ComandoRicevuto, ' ', 4); // DomoticzID

                CodiceArea = arg1.toInt() - 1;

                if (CodiceZona >= 0 && CodiceZona < MAX_AREE)
                {
                    StartupConfig.Area[CodiceArea].Abilitato = (arg2.toInt() == 1);
                    StartupConfig.Area[CodiceArea].DomoticzIdx = arg4.toInt();

                    sprintf(TraceString, "Modificata Area <%d> - Abilitata <%d> - DomoticzID <%d>",
                            CodiceArea + 1,
                            (arg2.toInt() == 1),
                            arg4.toInt());
                }
                else
                {
                    sprintf(TraceString, "Errore CodiceArea <%d>", CodiceZona);
                }
            }
            else
            {
                sprintf(TraceString, "Errore Polling Attivo!");
            }
        }
        else if (SecondoComando == String("param"))
        {
            // non si può effettuare la configurazione con polling attivo
            if (!StartupConfig.PollingKyo)
            {
                arg1 = getValue(ComandoRicevuto, ' ', 2); // PARAMETRO
                arg2 = getValue(ComandoRicevuto, ' ', 3); // VALORE

                if (arg1 == "DomoticzUpdate")
                {
                    StartupConfig.DomoticzUpdate = (arg2.toInt() == 1);
                    sprintf(TraceString, "Modificato Parametro <DomoticzUpdate> - Valore <%d>", StartupConfig.DomoticzUpdate);
                }
                else if (arg1 == "DomoticzStateIdx")
                {
                    StartupConfig.DomoticzStateIdx = arg2.toInt();
                    sprintf(TraceString, "Parametro <DomoticzStateIdx> - Valore <%d>", StartupConfig.DomoticzStateIdx);
                }
                else if (arg1 == "MancanzaRete")
                {
                    StartupConfig.Warnings.MancanzaRete.DomoticzIdx = arg2.toInt();
                    sprintf(TraceString, "Parametro <MancanzaRete.DomoticzIdx> - Valore <%d>", StartupConfig.Warnings.MancanzaRete.DomoticzIdx);
                }
                else if (arg1 == "StatoPiano")
                {
                    StartupConfig.Warnings.StatoPiano.DomoticzIdx = arg2.toInt();
                    sprintf(TraceString, "Parametro <StatoPiano.DomoticzIdx> - Valore <%d>", StartupConfig.Warnings.StatoPiano.DomoticzIdx);
                }
                else if (arg1 == "StatoMansarda")
                {
                    StartupConfig.Warnings.StatoMansarda.DomoticzIdx = arg2.toInt();
                    sprintf(TraceString, "Parametro <StatoMansarda.DomoticzIdx> - Valore <%d>", StartupConfig.Warnings.StatoMansarda.DomoticzIdx);
                }
                else if (arg1 == "StatoPerimetro")
                {
                    StartupConfig.Warnings.StatoPerimetro.DomoticzIdx = arg2.toInt();
                    sprintf(TraceString, "Parametro <StatoPerimetro.DomoticzIdx> - Valore <%d>", StartupConfig.Warnings.StatoPerimetro.DomoticzIdx);
                }
                else
                {
                    sprintf(TraceString, "Errore param non valido!");
                }
            }
            else
            {
                sprintf(TraceString, "Errore Polling Attivo!");
            }
        }

        // Risposta comando su MQTT
        client.publish(trace_topic, TraceString);
    }
    //
    // Comando "show"
    //
    else if (Comando == String("show"))
    {
        // SHOW - primo comando
        //    ZONE <NumeroZona>
        //    PARAM <PARAMETRO>
        SecondoComando = getValue(ComandoRicevuto, ' ', 1); // SecondoComando

        sprintf(TraceString, "Errore Show!");

        if (SecondoComando == String("?"))
        {
            sprintf(TraceString, "show zone <NumeroZona>\nshow param <?/PARAMETRO>");
        }
        else if (SecondoComando == String("zone"))
        {
            arg1 = getValue(ComandoRicevuto, ' ', 2); // NumeroZona

            CodiceZona = arg1.toInt() - 1;

            if (CodiceZona >= 0 && CodiceZona < MAX_ZONE)
            {
                sprintf(TraceString, "Zona <%d> - Abilitata <%d> - Stato Attivo <%d> - DomoticzID <%d> -> LastStatus = %d",
                        CodiceZona + 1,
                        StartupConfig.Zone[CodiceZona].Abilitata,
                        StartupConfig.Zone[CodiceZona].StatoAttivo,
                        StartupConfig.Zone[CodiceZona].DomoticzID,
                        StartupConfig.Zone[CodiceZona].LastStatus);
            }
            else
            {
                sprintf(TraceString, "Errore CodiceZona <%d>", CodiceZona);
            }
        }
        else if (SecondoComando == String("param"))
        {
            arg1 = getValue(ComandoRicevuto, ' ', 2); // Nome Parametro

            if (arg1 == "DomoticzUpdate")
            {
                sprintf(TraceString, "Parametro <DomoticzUpdate> - Valore <%d>", StartupConfig.DomoticzUpdate);
            }
            else if (arg1 == "DomoticzStateIdx")
            {
                sprintf(TraceString, "Parametro <DomoticzStateIdx> - Valore <%d>", StartupConfig.DomoticzStateIdx);
            }
            else if (arg1 == "MancanzaRete")
            {
                sprintf(TraceString, "Parametro <MancanzaRete.DomoticzIdx> - Valore <%d>", StartupConfig.Warnings.MancanzaRete.DomoticzIdx);
            }
        }

        // traccio risposta su MQTT
        client.publish(trace_topic, TraceString);
    }
    //
    // Comando "restart"
    //
    else if (Comando == String("restart"))
    {
        client.publish(trace_topic, "Restart ESP");
        ESP.restart();
    }

    //
    // Comando "reset"
    //
    else if (Comando == String("reset"))
    {
        client.publish(trace_topic, "Reset ESP");
        ESP.eraseConfig();
        ESP.reset();
    }

    //
    // Comando "start" Polling
    //
    else if (Comando == String("start"))
    {
        client.publish(trace_topic, "Start Polling Kyo");

        FasePollingKyo = FASE_POLLING_KYO_INIT;
        StartupConfig.PollingKyo = true;
    }

    //
    // Comando "stop" Polling
    //
    else if (Comando == String("stop"))
    {
        client.publish(trace_topic, "Stop Polling Kyo");
        StartupConfig.PollingKyo = false;
    }

    //
    // Comando "trace" Abilitate
    //
    else if (Comando == String("trace"))
    {
        client.publish(trace_topic, "Trace Polling Kyo");
        StartupConfig.TracePolling = true;
    }

    //
    // Comando "notrace" Disabilitate
    //
    else if (Comando == String("notrace"))
    {
        client.publish(trace_topic, "No Trace Polling Kyo");
        StartupConfig.TracePolling = false;
    }

    //
    // Comando "save" Configurazione
    //
    else if (Comando == String("save"))
    {
        if (SalvaConfigurazione())
            client.publish(trace_topic, "Configurazione Salvata in EEPROM");
        else
            client.publish(trace_topic, "Errore Salvataggio in EEPROM!");
    }

    //
    // Comando "default" Configurazione
    //
    else if (Comando == String("default"))
    {
        if (!StartupConfig.PollingKyo)
        {
            if (DefaultConfig())
                client.publish(trace_topic, "Ripristinata Configurazione");
            else
                client.publish(trace_topic, "Errore Ripristino Configurazione!");
        }
        else
        {
            client.publish(trace_topic, "Errore Polling Attivo!");
        }
    }

    //
    // Comando non gestito !
    //
    else
        client.publish(trace_topic, "Comando non Gestito!");

    return;
}
