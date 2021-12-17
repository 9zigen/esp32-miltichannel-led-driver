<template>
  <div class="container m-top-touch-navbar">
    <!-- Page Title -->
    <div class="tile is-ancestor is-marginless">
      <div class="tile is-parent">
        <div class="tile is-child text-left">
          <h1 class="title">
            Dashboard
          </h1>
        </div>
      </div>
    </div>

    <div class="tile is-ancestor is-marginless">
      <!-- Wrapper -->
      <div class="tile is-vertical">
        <div class="tile">
          <!-- left col -->
          <div class="tile is-vertical">
            <!-- Schedule chart -->
            <div class="tile is-parent">
              <article class="tile is-child box">
                <div class="content">
                  <p class="subtitle is-4">
                    Light Schedule
                  </p>
                  <div class="content">
                    <schedule-chart />
                  </div>
                </div>
              </article>
            </div>
          </div>
          <!-- Right col -->
          <div class="tile is-vertical">
            <!-- Light toggle -->
            <div class="tile is-parent">
              <article class="tile is-child box has-background-warning">
                <p class="subtitle is-4">
                  Light Channel control
                </p>
                <div class="columns is-vcentered">
                  <div class="column is-12">
                    <div
                      v-for="(led, index) in status.channels"
                      v-show="active[index]"
                      v-bind:key="index"
                      class="field is-horizontal"
                    >
                      <div class="field-body">
                        <div class="field">
                          <div class="control">
                            <slider
                              v-model.number="status.channels[index]"
                              min="0"
                              max="255"
                              v-bind:color="sliderColor(index)"
                              @change="setLight"
                            />
                          </div>
                        </div>
                      </div>
                    </div>
                    <div class="field is-horizontal">
                      <div class="field-body">
                        <div class="field">
                          <label>Brightness</label>
                          <div class="control">
                            <slider
                              v-model.number="status.brightness"
                              min="0"
                              max="100"
                              @change="setLight"
                            />
                          </div>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </article>
            </div>
            <!-- Light Channel status -->
            <div class="tile is-parent">
              <article class="tile is-child box">
                <p class="subtitle is-4">
                  Channels Power
                </p>
                <div class="tags are-medium">
                  <span
                    v-for="(led, index) in status.channels"
                    v-show="active[index]"
                    v-bind:key="index"
                    v-bind:style="{borderColor: sliderColor(index)}"
                    class="tag"
                  >
                    {{ ledPower(index) }}
                  </span>
                </div>
                <p><strong>Total: aprox. {{ totalPower() }} Watts</strong></p>
              </article>
            </div>
          </div>
        </div>

        <div class="tile">
          <div class="tile is-vertical">
            <!-- Hardware Status -->
            <div class="tile is-parent">
              <article class="tile is-child notification bg-notification is-primary">
                <p class="title">
                  Hardware
                </p>
                <p class="subtitle">
                  Device info
                </p>
                <div class="columns">
                  <div class="column has-text-left">
                    <ul>
                      <li>Firmware: {{ status.firmware }}</li>
                      <li>Hardware: {{ status.hardware }}</li>
                      <li>Free Heap: {{ status.freeHeap }}</li>
                      <li>WIFI: {{ status.wifiMode }}</li>
                      <li>IP Address: {{ status.ipAddress }}</li>
                      <li>MAC: {{ status.macAddress }}</li>
                    </ul>
                  </div>
                  <div class="column has-text-left">
                    <ul>
                      <li>Time: {{ status.localTime }}</li>
                      <li>Uptime: {{ status.upTime }}</li>
                      <li>Power IN: {{ status.vcc / 1000 }} volt.</li>
                      <li>NTC Temperature: {{ status.ntc_temperature }} °C</li>
                      <li>Board Temperature: {{ status.board_temperature / 100 }} °C</li>
                      <li>MQTT Server: {{ mqttStatus }}</li>
                      <li>NTP: {{ ntpStatus }}</li>
                    </ul>
                  </div>
                </div>
                <div class="columns">
                  <div class="column has-text-centered">
                    <div class="field is-grouped">
                      <div class="control">
                        <a
                          class="button is-light"
                          @click="updateDevice"
                        >Update</a>
                        <p class="help has-text-centered">
                          Firmware
                        </p>
                      </div>
                      <div class="control">
                        <a
                          class="button is-warning"
                          @click="rebootDevice"
                        >Restart</a>
                        <p class="help has-text-centered">
                          Device
                        </p>
                      </div>
                      <div class="control">
                        <a
                          class="button is-danger"
                          @click="restoreDevice"
                        >Factory<span class="is-hidden-mobile"> Reset</span></a>
                        <p class="help has-text-centered">
                          Initial conf.
                        </p>
                      </div>
                    </div>
                  </div>
                </div>
              </article>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>

import { eventBus } from '../eventBus'
import { http } from '../http'

export default {
  name: 'Home',
  data () {
    return {
      status: {
        upTime: '',
        localTime: '',
        freeHeap: 0,
        vcc: 0,
        ntc_temperature: 20,
        board_temperature: 20,
        wifiMode: '',
        ipAddress: '',
        macAddress: '00:00:00:00:00:00',
        mqttService: '',
        ntpService: '',
        channels: [0, 0, 0, 0, 0, 0, 0, 0],
        brightness: 0,
        firmware: '',
        hardware: ''
      },
      colors: [],
      power: [],
      active: [],
      refreshInterval: null,
      isLoading: true
    }
  },
  computed: {
    mqttStatus: function () {
      let status = this.status.mqttService.enabled ? 'on' : 'off'
      status += this.status.mqttService.connected ? ', connected' : ', disconnected'
      return status
    },
    ntpStatus: function () {
      let status = this.status.ntpService.enabled ? 'on' : 'off'
      status += this.status.ntpService.sync ? ', sync' : ', not sync'
      return status
    }
  },
  mounted () {
    this.requestData()
  },
  methods: {
    async requestData () {
      eventBus.$emit('loading', true)
      try {
        /* Device status */
        let statusResponse = await http.get('/status')
        this.status = statusResponse.data.status

        /* Leds config */
        let settingsResponse = await http.get('/api/settings')
        this.colors = settingsResponse.data.leds.filter(led => led.state === 1).map((value, index, array) => value.color)
        this.power = settingsResponse.data.leds.filter(led => led.state === 1).map((value, index, array) => value.power)
        this.active = settingsResponse.data.leds.map((led) => led.state)
      } catch (e) {
        if (e.response) {
          // eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
      eventBus.$emit('loading', false)
    },
    ledStatus (value) {
      if (value) { return `${parseInt(value)}%` } return `OFF ${parseInt(value)}%`
    },
    ledPower (index) {
      let string = ''
      const channels = this.status.channels

      if (!this.power[index]) {
        return 'POWER not SET '
      }

      if (channels[index]) {
        const percent = channels[index] / 255
        const power = this.power[index] * this.status.brightness / 100

        string += `${parseInt(percent * 100)}%`
        string += ` (${parseInt(percent * power)} Watts)`
        return string
      }

      return 'OFF '
    },
    totalPower () {
      const active = this.active
      const power = this.power
      const brightness = this.status.brightness / 100
      const channels = this.status.channels.filter((value, index) => active[index])
      let totalPow = 0

      if (channels) {
        totalPow = channels.reduce((total, duty, index) => total + duty / 255 * power[index] * brightness, 0)
      }

      return totalPow.toFixed(2)
    },
    async restoreDevice () {
      if (confirm('Do you really want to factory restore your device?')) {
        if (confirm('Sure?')) {
          try {
            let response = await http.get('/factory')
            if (response.data.success) {
              eventBus.$emit('message', 'Factory Restoring...', 'success')
            }
          } catch (e) {
            if (e.response) {
              eventBus.$emit('message', e.response.data.message, 'danger')
            } else {
              eventBus.$emit('message', 'unexpected error', 'danger')
            }
          }
        }
      }
    },
    async rebootDevice () {
      if (confirm('Do you really want to restart your device?')) {
        if (confirm('Sure?')) {
          try {
            let response = await http.get('/reboot')
            if (response.data.success) { eventBus.$emit('message', 'Rebooting...', 'success') }
          } catch (e) {
            if (e.response) {
              eventBus.$emit('message', e.response.data.message, 'danger')
            } else {
              eventBus.$emit('message', 'unexpected error', 'danger')
            }
          }
        }
      }
    },
    async updateDevice () {
      if (confirm('Do you really want to update firmware from cloud?')) {
        if (confirm('Sure?')) {
          try {
            let response = await http.get('/update')
            if (response.data.success) {
              eventBus.$emit('message', 'Update Firmware...', 'success')
            }
          } catch (e) {
            if (e.response) {
              eventBus.$emit('message', e.response.data.message, 'danger')
            } else {
              eventBus.$emit('message', 'unexpected error', 'danger')
            }
          }
        }
      }
    },
    async setLight () {
      if (this.status.channels.length > 0 && this.status.brightness !== undefined) {
        try {
          /* Set New Duty value */
          let response = await http.post('/api/light', {
            channels: this.status.channels,
            brightness: this.status.brightness })

          if (response.data.success) { eventBus.$emit('message', 'Applied...', 'success') }
        } catch (e) {
          if (e.response) {
            eventBus.$emit('message', e.response.data.message, 'danger')
          } else {
            eventBus.$emit('message', 'unexpected error', 'danger')
          }
        }
      }
    },
    sliderColor (index) {
      return this.colors[index]
    }
  }
}
</script>

<style lang="scss">
.tag {
  border: dashed 1px;
  border-bottom: solid 0.25em;
  box-sizing: border-box;
  padding-left: 0.25em !important;
  padding-right: 0.25em !important;
}
</style>
