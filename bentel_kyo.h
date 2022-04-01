/*
* espkyogate - ESPHome custom component for Bentel KYO alarms
* Copyright (C) 2022 Lorenzo De Luca (me@lorenzodeluca.dev)
* 
* This program is distributed in the hope that it will be useful, but WITHOUT ANY
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
* PARTICULAR PURPOSE. See the GNU General Public License for more details.
* 
*
* GNU Affero General Public License v3.0
*/

#include <sstream>
#include "esphome.h"
#include "esp8266_mutex.h"

#define KYO_MODEL_4   "KYO4"
#define KYO_MODEL_8   "KYO8"
#define KYO_MODEL_8G  "KYO8G"
#define KYO_MODEL_32  "KYO32"
#define KYO_MODEL_32G "KYO32G"
#define KYO_MODEL_8W  "KYO8 W"
#define KYO_MODEL_8GW "KYO8G W"

#define KYO_MAX_ZONE 32
#define KYO_MAX_AREE 8

#define UPDATE_INT_MS 1000

class BentelKyoComponent : public esphome::PollingComponent, public uart::UARTDevice, public api::CustomAPIDevice {
	public:
		BentelKyoComponent(UARTComponent *parent) : UARTDevice(parent) {}
		
		TextSensor *alarmStatusSensor = new TextSensor();
		TextSensor *modelSensor = new TextSensor();
		TextSensor *firmwareSensor = new TextSensor();
		
		BinarySensor *kyo_comunication = new BinarySensor();

		BinarySensor *warn_mancanza_rete = new BinarySensor();
		BinarySensor *warn_scomparsa_bpi = new BinarySensor();
		BinarySensor *warn_fusibile = new BinarySensor();
		BinarySensor *warn_batteria_bassa = new BinarySensor();
		BinarySensor *warn_guasto_linea_telefonica = new BinarySensor();
		BinarySensor *warn_codici_default = new BinarySensor();
		BinarySensor *warn_wireless = new BinarySensor();

		BinarySensor *stato_sirena = new BinarySensor();
		BinarySensor *sabotaggio_zona = new BinarySensor();
		BinarySensor *sabotaggio_chiave_falsa = new BinarySensor();
		BinarySensor *sabotaggio_bpi = new BinarySensor();
		BinarySensor *sabotaggio_sistema = new BinarySensor();
		BinarySensor *sabotaggio_jam = new BinarySensor();
		BinarySensor *sabotaggio_wireless = new BinarySensor();

		BinarySensor* zona = new BinarySensor[KYO_MAX_ZONE];
		BinarySensor* zona_sabotaggio = new BinarySensor[KYO_MAX_ZONE];
		
		// IDLE, ESCLUSA, MEMORIA_ALLARMI, MEMORIA_SABOTAGGIO
		TextSensor* partition_status = new TextSensor[KYO_MAX_ZONE];
		
		BinarySensor* allarme_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_totale_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_parziale_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* inserimento_parziale_ritardo_0_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* disinserita_area = new BinarySensor[KYO_MAX_AREE];
		BinarySensor* stato_uscita = new BinarySensor[KYO_MAX_AREE];

		void setup() override
		{
			pollingState = PollingStateEnum::Init;

			set_update_interval(UPDATE_INT_MS);
			set_setup_priority(setup_priority::AFTER_CONNECTION);

			CreateMutux(&uartMutex);

			register_service(&BentelKyoComponent::arm_area, "arm_area", {"area", "arm_type", "specific_area"});
			register_service(&BentelKyoComponent::disarm_area, "disarm_area", {"area", "specific_area"});
			register_service(&BentelKyoComponent::reset_alarms, "reset_alarms");
			register_service(&BentelKyoComponent::activate_output, "activate_output", {"output_number"});
			register_service(&BentelKyoComponent::deactivate_output, "deactivate_output", {"output_number"});
			register_service(&BentelKyoComponent::debug_command, "debug_command", {"serial_trace", "log_trace"});

			alarmStatusSensor->publish_state("unavailable");
			kyo_comunication->publish_state(false);

			ESP_LOGCONFIG("setup", "Bentel Kyo Setup");
		}

		void arm_area(int area, int arm_type, int specific_area)
		{
			if (area > KYO_MAX_AREE)
			{
				ESP_LOGE("arm_area", "Invalid Area %i, MAX %i", area, KYO_MAX_AREE);
				return;
			}
			
			ESP_LOGCONFIG("arm_area", "request arm type %d area %d", arm_type, area);
			byte cmdArmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xCC, 0xFF};

			byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

			if (specific_area == 1)
			{
				for (int i = 0; i < KYO_MAX_AREE; i++)
				{
					total_insert_area_status |= (this->inserimento_totale_area[i].state) << i;
					partial_insert_area_status |= (this->inserimento_parziale_area[i].state) << i;
				}
			}

			if (arm_type == 2)
				partial_insert_area_status |= 1 << (area - 1);
			else
				total_insert_area_status |= 1 << (area - 1);

			cmdArmPartition[6] = total_insert_area_status;
			cmdArmPartition[7] = partial_insert_area_status;
			cmdArmPartition[9] = calculateCRC(cmdArmPartition, 8);

			byte Rx[255];
			int Count = sendMessageToKyo(cmdArmPartition, sizeof(cmdArmPartition), Rx, 100);
			ESP_LOGCONFIG("arm_area", "arm_area kyo respond %i", Count);
		}

		void disarm_area(int area, int specific_area)
		{
			if (area > KYO_MAX_AREE)
			{
				ESP_LOGE("arm_area", "invalid Area %i, MAX %i", area, KYO_MAX_AREE);
				return;
			}
			
			ESP_LOGCONFIG("disarm_area", "request disarm area %d", area);
			byte cmdDisarmPartition[11] = {0x0F, 0x00, 0xF0, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0xFF, 0xFF};

			byte total_insert_area_status = 0x00, partial_insert_area_status = 0x00;

			if (specific_area == 1)
			{
				for (int i = 0; i < KYO_MAX_AREE; i++)
				{
					total_insert_area_status |= (this->inserimento_totale_area[i].state) << i;
					partial_insert_area_status |= (this->inserimento_parziale_area[i].state) << i;
				}

				if (this->inserimento_totale_area[area - 1].state)
					total_insert_area_status |= 0 << (area - 1);
				else
					partial_insert_area_status |= 1 << (area - 1);
			}

			cmdDisarmPartition[6] = total_insert_area_status;
			cmdDisarmPartition[7] = partial_insert_area_status;
			cmdDisarmPartition[9] = calculateCRC(cmdDisarmPartition, 8);

			byte Rx[255];
			int Count = sendMessageToKyo(cmdDisarmPartition, sizeof(cmdDisarmPartition), Rx, 80);
			ESP_LOGCONFIG("disarm_area", "kyo respond %i", Count);
		}

		void reset_alarms()
		{
			ESP_LOGE("reset_alarms", "Reset Alarms.");

			byte Rx[255];
			int Count = sendMessageToKyo(cmdResetAllarms, sizeof(cmdResetAllarms), Rx, 80);
			ESP_LOGE("reset_alarms", "kyo respond %i", Count);
		}

		void debug_command(int serial_trace, int log_trace)
		{
			this->serialTrace = (serial_trace == 1);
			this->logTrace = (log_trace == 1);

			ESP_LOGE("debug_command", "serial_trace %i log_trace %i", this->serialTrace, this->logTrace);
		}

		void activate_output(int output_number)
		{
			if (output_number > KYO_MAX_AREE)
			{
				ESP_LOGE("activate_output", "invalid output %i, MAX %i", output_number, KYO_MAX_AREE);
				return;
			}

			ESP_LOGE("activate_output", "activate Output Number: %d", output_number);
			
			byte cmdActivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0x00};
			
			cmdActivateOutput[6] |= 1 << (output_number - 1);
			cmdActivateOutput[8] = cmdActivateOutput[6];

			byte Rx[255];
			int Count = sendMessageToKyo(cmdActivateOutput, sizeof(cmdActivateOutput), Rx, 80);
			ESP_LOGD("activate_output", "kyo respond %i", Count);
		}

		void deactivate_output(int output_number)
		{
			if (output_number > KYO_MAX_AREE)
			{
				ESP_LOGI("deactivate_output", "invalid output %i, MAX %i", output_number, KYO_MAX_AREE);
				return;
			}

			ESP_LOGD("deactivate_output", "deactivate Output Number: %d", output_number);
			
			byte cmdDeactivateOutput[9] = {0x0f, 0x06, 0xf0, 0x01, 0x00, 0x06, 0x00, 0x00, 0xCC};
			
			cmdDeactivateOutput[7] |= 1 << (output_number - 1);
			cmdDeactivateOutput[8] = cmdDeactivateOutput[7];

			byte Rx[255];
			int Count = sendMessageToKyo(cmdDeactivateOutput, sizeof(cmdDeactivateOutput), Rx, 80);
			ESP_LOGD("deactivate_output", "kyo respond %i", Count);
		}

		void update() override
		{
			if (GetMutex(&uartMutex) == true) {

                switch(this->pollingState)
				{
					case PollingStateEnum::Init:
						if (this->update_kyo_status())
						{
							this->pollingState = PollingStateEnum::Status;
							this->centralInvalidMessageCount = 0;
						}
						else
							this->centralInvalidMessageCount++;

						break;

					case PollingStateEnum::Status:
						if (this->update_kyo_partitions())
						{
							this->pollingState = PollingStateEnum::Init;
							this->centralInvalidMessageCount = 0;
						}
						else
							this->centralInvalidMessageCount++;
				
						break;
				}

				// Release UART mutex
				ReleaseMutex(&uartMutex);
			}

			if (this->centralInvalidMessageCount == 0 && !this->kyo_comunication->state)
				this->kyo_comunication->publish_state(true);
			else if(centralInvalidMessageCount > 3)
				this->kyo_comunication->publish_state(false);
		}

	private:

		byte cmdGetSensorStatus[6] = {0xf0, 0x04, 0xf0, 0x0a, 0x00, 0xee};	  // Read Realtime Status and Trouble Status
		byte cmdGetPartitionStatus[6] = {0xf0, 0x02, 0x15, 0x12, 0x00, 0x19}; // Partitions Status (305) - Outputs Status - Tamper Memory - Bypassed Zones - Zone Alarm Memory - Zone Tamper Memory
		byte cmqGetSoftwareVersion[6] = {0xf0, 0x00, 0x00, 0x0b, 0x00, 0xfb}; // f0 00 00 0b 00 fb
		byte cmdResetAllarms[9] = {0x0F, 0x05, 0xF0, 0x01, 0x00, 0x05, 0x07, 0x00, 0x07};

		const std::vector<uint8_t> cmdGetAlarmInfo = {0xf0, 0x00, 0x00, 0x0b, 0x00};
        const size_t RPL_GET_ALARM_INFO_SIZE = 13;

		enum class AlarmModel {UNKNOWN, KYO_4, KYO_8, KYO_8G, KYO_32, KYO_32G, KYO_8W, KYO_8GW};
        AlarmModel alarmModel = AlarmModel::UNKNOWN;

        enum class AlarmStatus {UNAVAILABLE, PENDING, ARMING, ARMED_AWAY, ARMED_HOME, DISARMED, TRIGGERED};
        AlarmStatus alarmStatus = AlarmStatus::UNAVAILABLE;

		enum class PollingStateEnum { Init = 1, Status };
		PollingStateEnum pollingState;

		enum class PartitionStatusEnum { Idle = 1, Exclude, MemAlarm, MemSabotate};
		PartitionStatusEnum PartitionStatusInternal[KYO_MAX_ZONE];

		mutex_t uartMutex;
		bool serialTrace = false;
		bool logTrace = false;
		int centralInvalidMessageCount = 0;

		bool update_kyo_partitions()
		{
			byte Rx[255];
			int Count = 0;

			Count = sendMessageToKyo(cmdGetPartitionStatus, sizeof(cmdGetPartitionStatus), Rx, 100);
			if (Count != 26)
			{
				if (this->logTrace)
					ESP_LOGE("update_kyo_partitions", "invalid message length %i", Count);

				return (false);
			}

			int StatoZona, i;
			for(i = 0; i < KYO_MAX_ZONE; i++)
				this->PartitionStatusInternal[i] = PartitionStatusEnum::Idle;

			// Ciclo AREE INSERITE
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[6] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_totale_area[i].state)
					ESP_LOGE("aree_totale", "Area %i - Stato %i", i, StatoZona);
				
				inserimento_totale_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE INSERITE PARZIALI
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[7] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_parziale_area[i].state)
					ESP_LOGE("aree_parziale", "Area %i - Stato %i", i, StatoZona);

				inserimento_parziale_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE INSERITE PARZIALI RITARDO 0
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[8] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != inserimento_parziale_ritardo_0_area[i].state)
					ESP_LOGE("inserimento_parziale_ritardo_0_area", "Area %i - Stato %i", i, StatoZona);

				inserimento_parziale_ritardo_0_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo AREE DISINSERITE
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[9] >> i) & 1;
				if (this->logTrace && (StatoZona == 1) != disinserita_area[i].state)
					ESP_LOGE("disinserita_area", "Area %i - Stato %i", i, StatoZona);

				disinserita_area[i].publish_state(StatoZona == 1);
			}

			// STATO SIRENA
			StatoZona = ((Rx[10] >> 5) & 1);
			if (this->logTrace && (StatoZona == 1) != stato_sirena->state)
				ESP_LOGE("stato_sirena", "Stato %i", StatoZona);
			stato_sirena->publish_state(StatoZona == 1);
			

			// CICLO STATO USCITE
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = 0;
				if (i >= 8 && i <= 15)
					StatoZona = (Rx[11] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[12] >> i) & 1;

				if (this->logTrace && (StatoZona == 1) != stato_uscita[i].state)
					ESP_LOGE("stato_uscita", "Uscita %i - Stato %i", i, StatoZona);

				stato_uscita[i].publish_state(StatoZona == 1);
			}

			// CICLO ZONE ESCLUSE
			for (i = 0; i < KYO_MAX_ZONE; i++)
			{
				StatoZona = 0;
				if (i >= 24)
					StatoZona = (Rx[13] >> (i - 24)) & 1;
				else if (i >= 16 && i <= 23)
					StatoZona = (Rx[14] >> (i - 16)) & 1;
				else if (i >= 8 && i <= 15)
					StatoZona = (Rx[15] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[16] >> i) & 1;

				//zona_esclusa[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::Exclude;
			}

			// CICLO MEMORIA ALLARME ZONE
			for (i = 0; i < KYO_MAX_ZONE; i++)
			{
				StatoZona = 0;
				if (i >= 24)
					StatoZona = (Rx[17] >> (i - 24)) & 1;
				else if (i >= 16 && i <= 23)
					StatoZona = (Rx[18] >> (i - 16)) & 1;
				else if (i >= 8 && i <= 15)
					StatoZona = (Rx[19] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[20] >> i) & 1;

				//memoria_allarme_zona[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::MemAlarm;
			}

			// CICLO MEMORIA SABOTAGGIO ZONE
			for (i = 0; i < KYO_MAX_ZONE; i++)
			{
				StatoZona = 0;
				if (i >= 24)
					StatoZona = (Rx[21] >> (i - 24)) & 1;
				else if (i >= 16 && i <= 23)
					StatoZona = (Rx[22] >> (i - 16)) & 1;
				else if (i >= 8 && i <= 15)
					StatoZona = (Rx[23] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[24] >> i) & 1;

				//memoria_sabotaggio_zona[i].publish_state(StatoZona == 1);
				if (StatoZona == 1)
					this->PartitionStatusInternal[i] = PartitionStatusEnum::MemSabotate;
			}

			for(i = 0; i < KYO_MAX_ZONE; i++)
			{
				switch(this->PartitionStatusInternal[i])
				{
					case PartitionStatusEnum::Idle:
						this->partition_status[i].publish_state("I");
						break;

					case PartitionStatusEnum::MemAlarm:
						this->partition_status[i].publish_state("A");
						break;

					case PartitionStatusEnum::Exclude:
						this->partition_status[i].publish_state("E");
						break;

					case PartitionStatusEnum::MemSabotate:
						this->partition_status[i].publish_state("S");
						break;
				}
				
				//if (this->logTrace && (StatoZona == 1) != memoria_sabotaggio_zona[i].state)	
				//	ESP_LOGD("memoria_sabotaggio_zona", "Zona %i - Stato %i", i, StatoZona);
			}
			
			return true;
		}

		bool update_kyo_status()
		{
			byte Rx[255];
			int Count = 0;

			Count = sendMessageToKyo(cmdGetSensorStatus, sizeof(cmdGetSensorStatus), Rx, 100);
			if (Count != 18)
			{
				if (this->logTrace)
					ESP_LOGE("update_kyo_status", "invalid message length %i", Count);
				
				return false;
			}

			int StatoZona, i;

			// Ciclo ZONE
			for (i = 0; i < KYO_MAX_ZONE; i++)
			{
				StatoZona = 0;
				if (i >= 24)
					StatoZona = (Rx[6] >> (i - 24)) & 1;
				else if (i >= 16 && i <= 23)
					StatoZona = (Rx[7] >> (i - 16)) & 1;
				else if (i >= 8 && i <= 15)
					StatoZona = (Rx[8] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[9] >> i) & 1;

				if (this->logTrace && (StatoZona == 1) != zona[i].state)	
					ESP_LOGE("stato_zona", "Zona %i - Stato %i", i, StatoZona);

				zona[i].publish_state(StatoZona==1);
			}

			// Ciclo SABOTAGGIO ZONE
			for (i = 0; i < KYO_MAX_ZONE; i++)
			{
				StatoZona = 0;
				if (i >= 24)
					StatoZona = (Rx[10] >> (i - 24)) & 1;
				else if (i >= 16 && i <= 23)
					StatoZona = (Rx[11] >> (i - 16)) & 1;
				else if (i >= 8 && i <= 15)
					StatoZona = (Rx[12] >> (i - 8)) & 1;
				else if (i <= 7)
					StatoZona = (Rx[13] >> i) & 1;

				zona_sabotaggio[i].publish_state(StatoZona == 1);
			}

			// Ciclo ALLARME AREA
			for (i = 0; i < KYO_MAX_AREE; i++)
			{
				StatoZona = (Rx[15] >> i) & 1;
				allarme_area[i].publish_state(StatoZona == 1);
			}

			// Ciclo WARNINGS
			for (i = 0; i < 8; i++)
			{
				StatoZona = (Rx[14] >> i) & 1;
				switch(i)
				{
					case 0:
						warn_mancanza_rete->publish_state(StatoZona == 1);
						break;

					case 1:
						warn_scomparsa_bpi->publish_state(StatoZona == 1);
						break;

					case 2:
						warn_fusibile->publish_state(StatoZona == 1);
						break;

					case 3:
						warn_batteria_bassa->publish_state(StatoZona == 1);
						break;

					case 4:
						warn_guasto_linea_telefonica->publish_state(StatoZona == 1);
						break;

					case 5:
						warn_codici_default->publish_state(StatoZona == 1);
						break;

					case 6:
						warn_wireless->publish_state(StatoZona == 1);
						break;
				}
			}

			// Ciclo SABOTAGGI
			for (i = 0; i < 8; i++)
			{
				StatoZona = (Rx[16] >> i) & 1;
				switch(i)
				{
					case 2:
						sabotaggio_zona->publish_state(StatoZona == 1);
						break;

					case 3:
						sabotaggio_chiave_falsa->publish_state(StatoZona == 1);
						break;

					case 4:
						sabotaggio_bpi->publish_state(StatoZona == 1);
						break;

					case 5:
						sabotaggio_sistema->publish_state(StatoZona == 1);
						break;
					
					case 6:
						sabotaggio_jam->publish_state(StatoZona == 1);
						break;

					case 7:
						sabotaggio_wireless->publish_state(StatoZona == 1);
						break;
				}
			}

			return true;
		}
		
		uint8_t getChecksum(const std::vector<uint8_t> &data, size_t offset = 0) {
            uint8_t ckSum = 0;

            for (int i = offset; i < data.size(); i++) {
                ckSum += data[i];
            }

            return (ckSum);
        }

		void appendChecksum(std::vector<uint8_t> &data, size_t offset = 0) {
            data.push_back(getChecksum(const_cast<std::vector<uint8_t>&>(data), offset));
        }

 		bool getAlarmInfo() {
            std::vector<uint8_t> request = cmdGetAlarmInfo;
            std::vector<uint8_t> reply;

            appendChecksum(request);

            if (sendRequest(request, reply, 100) && reply.size() == RPL_GET_ALARM_INFO_SIZE) {
                // Parse reply
                std::ostringstream convert;
                for (int i = 0; i < RPL_GET_ALARM_INFO_SIZE - 1; i++) {
                    convert << reply[i];
                }

                // Trim spaces from model sub-string
                std::string model = convert.str().substr(0, 7);
                rtrim(model);

                // Trim spaces from firmware substring
                std::string firmware = convert.str().substr(8, 11);
                rtrim(firmware);

                modelSensor->publish_state(model.c_str());
                firmwareSensor->publish_state(firmware.c_str());

                if (model == KYO_MODEL_4) alarmModel = AlarmModel::KYO_4;
                else if (model == KYO_MODEL_8) alarmModel = AlarmModel::KYO_8;
                else if (model == KYO_MODEL_8G) alarmModel = AlarmModel::KYO_8G;
                else if (model == KYO_MODEL_32) alarmModel = AlarmModel::KYO_32;
                else if (model == KYO_MODEL_32G) alarmModel = AlarmModel::KYO_32G;
                else if (model == KYO_MODEL_8W) alarmModel = AlarmModel::KYO_8W;
                else if (model == KYO_MODEL_8GW) alarmModel = AlarmModel::KYO_8GW;
                else alarmModel = AlarmModel::UNKNOWN;

                ESP_LOGCONFIG("getAlarmInfo", "KYO model request completed [%s %s]", model.c_str(), firmware.c_str());
                return (true);
            }

            ESP_LOGE("getAlarmInfo", "KYO model request failed");
            return (false);
        }
		
		int sendMessageToKyo(byte *cmd, int lcmd, byte ReadByes[], int waitForAnswer = 0)
		{
			// clean rx buffer
			while (available() > 0)
				read();

			delay(10);

			int index = 0;
			byte RxBuff[255];
			memset(ReadByes, 0, 254);

			write_array(cmd, lcmd);
			delay(waitForAnswer);

			if (available() >= 6) {
			
				// Read a single Byte
				while (available() > 0)
					RxBuff[index++] = read(); 

				if (this->serialTrace)
				{
					/*
					int i;
					char txString[255];
					char rxString[255];
					memset(txString, 0, 255);
					memset(rxString, 0, 255);

					for (i = 0; i < lcmd; i++)
						sprintf(txString, "%s %2x", txString, cmd[i]);

					for (i = 0; i < index; i++)
						sprintf(rxString, "%s %2x", rxString, RxBuff[i]);

					ESP_LOGD("sendMessageToKyo", "TX [%d] '%s', RX [%d] '%s'", lcmd, txString, index, rxString);
					*/
					ESP_LOGE("sendMessageToKyo", "TX [%d] '%s', RX [%d] '%s'", lcmd, format_hex_pretty(cmd, lcmd).c_str(),
													index, format_hex_pretty(RxBuff, index).c_str());
				}
			}

			if (index <= 0)
				return -1;

			memcpy(ReadByes, RxBuff, index);
			return index;
		}

		byte calculateCRC(byte *cmd, int lcmd)
		{
			int sum = 0x00;
			for (int i = 0; i <= lcmd; i++)
				sum += cmd[i];

			return (0x203 - sum);
		}

		bool verifyChecksum(const std::vector<uint8_t> &data, size_t offset = 0) {
            uint8_t ckSum = 0;

            for (int i = offset; i < data.size() - 1; i++) {
                ckSum += data[i];
            }

            return (data.back() == ckSum);
        }

        static inline void rtrim(std::string &str) {
            str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(), str.end());
        }

        bool sendRequest(std::vector<uint8_t> request, std::vector<uint8_t> &reply, uint wait = 0) {
            ESP_LOGD("sendRequest", "Request: %s", format_hex_pretty(request).c_str());

            // Empty receiveing buffer
            while (available() > 0) {
                read();
            }

            // Send request
            write_array(reinterpret_cast<byte*> (request.data()), request.size());

            delay(wait);

            // Read reply
            reply.clear();

            // Wait for request echo
            if (available() >= 6) {
                while (available() > 0) {
                    reply.push_back(read());
                }

                // Strip request echo
                reply.erase(reply.begin(), reply.begin() + 6);

                if (reply.size() > 0) {
                    ESP_LOGD("sendRequest", "Reply: %s", format_hex_pretty(reply).c_str());

                    // Verify checksum
                    return (verifyChecksum(const_cast<std::vector<uint8_t>&>(reply)));
                }

                return (true);
            }

            return (false);
        }
};
