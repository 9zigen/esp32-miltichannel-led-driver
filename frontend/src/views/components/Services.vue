<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12">
      <!-- MAIN -->
      <div class="notification bg-notification is-light">
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Identity</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="services.hostname"
                  class="input"
                  type="text"
                  placeholder="Hostname"
                >
              </div>
            </div>
          </div>
        </div>

        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">OTA url</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="services.ota_url"
                  class="input"
                  type="text"
                  placeholder="OTA Url"
                >
              </div>
            </div>
            <div class="field">
              <div class="control">
                <span
                  class="button is-outlined"
                  @click="updateDevice"
                >
                  <check-icon
                    size="1.5x"
                    class="custom-class"
                  /> Apply
                </span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- NTP -->
      <div class="notification bg-notification is-light">
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">NTP</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control has-text-centered">
                <toggle-switch
                  v-model="services.enable_ntp"
                  round
                />
              </div>
              <p class="help">
                Enable
              </p>
            </div>
            <div class="field">
              <div class="control has-text-centered">
                <toggle-switch
                  v-model="services.ntp_dst"
                  round
                />
              </div>
              <p class="help">
                Daylight saving time
              </p>
            </div>
            <div class="field">
              <div class="control">
                <input
                  v-model="services.ntp_server"
                  class="input"
                  type="text"
                  placeholder="NTP Server Name"
                >
              </div>
              <p class="help">
                Server Name
              </p>
            </div>
            <div class="field">
              <div class="control">
                <div class="select">
                  <select
                    v-model.number="services.utc_offset"
                  >
                    <option
                      v-for="(option, index) in tzOptions"
                      v-bind:key="index"
                      v-bind:value="option.value"
                    >
                      {{ option.text }}
                    </option>
                  </select>
                </div>
              </div>
              <p class="help">
                TimeZone
              </p>
            </div>
          </div>
        </div>
      </div>

      <!-- MQTT -->
      <div class="notification bg-notification is-light">
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">MQTT</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control has-text-centered">
                <toggle-switch
                  v-model="services.enable_mqtt"
                  round
                />
              </div>
              <p class="help">
                Enable
              </p>
            </div>
            <div class="field">
              <div class="control">
                <div class="select">
                  <select
                    v-model.number="services.mqtt_qos"
                    name="mqtt_qos"
                  >
                    <option
                      v-for="(option, index) in mqttOptions"
                      v-bind:key="index"
                      v-bind:value="option.value"
                    >
                      {{ option.text }}
                    </option>
                  </select>
                </div>
              </div>
              <p class="help">
                MQTT QoS
              </p>
            </div>
            <div class="field">
              <div class="control">
                <input-ip
                  v-model="services.mqtt_ip_address"
                  placeholder="MQTT IP Address"
                />
              </div>
              <p class="help">
                MQTT Server IP Address
              </p>
            </div>
            <div class="field">
              <div class="control">
                <input
                  v-model="services.mqtt_port"
                  class="input"
                  type="text"
                  placeholder="MQTT Server Port"
                >
              </div>
              <p class="help">
                MQTT Server Port
              </p>
            </div>
          </div>
        </div>
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label" />
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="services.mqtt_user"
                  class="input"
                  type="text"
                  placeholder="User"
                >
              </div>
              <p class="help">
                MQTT User
              </p>
            </div>
            <div class="field">
              <div class="control">
                <input
                  v-model="services.mqtt_password"
                  class="input"
                  type="text"
                  placeholder="Password"
                >
              </div>
              <p class="help">
                MQTT Password
              </p>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div class="column">
      <!-- Buttons -->
      <div class="buttons is-centered">
        <span
          class="button is-primary"
          @click="saveServices"
        >
          <check-icon
            size="1.5x"
            class="custom-class"
          /> Apply
        </span>
        <span
          class="button is-danger"
          @click="loadServices"
        >
          <x-icon
            size="1.5x"
            class="custom-class"
          /> Cancel
        </span>
      </div>
    </div>
  </div>
</template>

<script>
import { eventBus } from '@/eventBus'
import { http } from '@/http'

export default {
  name: 'Services',
  data () {
    return {
      services: {
        hostname: '',
        ota_url: 'http://192.168.4.2:8080',
        ntp_server: '',
        utc_offset: 0,
        ntp_dst: true,
        mqtt_ip_address: '',
        mqtt_port: '',
        mqtt_user: '',
        mqtt_password: '',
        mqtt_qos: 0,
        enable_ntp: false,
        enable_mqtt: false
      },
      mqttOptions: [
        { text: 'At most once (0)', value: 0 },
        { text: 'At least once (1)', value: 1 },
        { text: 'Exactly once (2)', value: 2 }
      ],
      tzOptions: [
        { text: 'GMT-14', value: -14 },
        { text: 'GMT-12', value: -12 },
        { text: 'GMT-11', value: -11 },
        { text: 'GMT-10', value: -10 },
        { text: 'GMT-9', value: -9 },
        { text: 'GMT-8', value: -8 },
        { text: 'GMT-7', value: -7 },
        { text: 'GMT-6', value: -6 },
        { text: 'GMT-5', value: -5 },
        { text: 'GMT-4', value: -4 },
        { text: 'GMT-3', value: -3 },
        { text: 'GMT-2', value: -2 },
        { text: 'GMT-1', value: -1 },
        { text: 'GMT', value: 0 },
        { text: 'GMT+1', value: 1 },
        { text: 'GMT+2', value: 2 },
        { text: 'GMT+3', value: 3 },
        { text: 'GMT+4', value: 4 },
        { text: 'GMT+5', value: 5 },
        { text: 'GMT+6', value: 6 },
        { text: 'GMT+7', value: 7 },
        { text: 'GMT+8', value: 8 },
        { text: 'GMT+9', value: 9 },
        { text: 'GMT+10', value: 10 },
        { text: 'GMT+11', value: 11 },
        { text: 'GMT+12', value: 12 }
      ]
    }
  },
  mounted () {
    this.loadServices()
  },
  methods: {
    async saveServices () {
      let response = await http.post('/api/settings', { services: this.services })
      if (response.data.success) {
        eventBus.$emit('message', 'Saved', 'success')
      } else {
        eventBus.$emit('message', 'NOT Saved', 'danger')
      }
    },
    async loadServices () {
      let response = await http.get('/api/settings')
      this.services = response.data.services
    },
    async updateDevice () {
      try {
        let response = await http.get('/update')
        if (response.data.success) { eventBus.$emit('message', 'Update Firmware...', 'success') }
      } catch (e) {
        eventBus.$emit('message', e, 'danger')
      }
    }
  }
}
</script>
