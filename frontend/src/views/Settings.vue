<template>
  <div class="container m-top-touch-navbar">
    <div class="columns is-marginless">
      <div class="column">
        <div class="content has-text-left">
          <h1 class="title">
            Settings
          </h1>
        </div>
      </div>
    </div>

    <div class="columns is-marginless">
      <div class="column">
        <!-- MAIN -->
        <div class="buttons has-addons mobile-scrollable">
          <span
            v-for="(tab, idx) in tabs"
            v-bind:key="tab"
            v-bind:class="['button', { 'is-primary': currentTab === tab }]"
            @click="selectTab(idx)"
          >
            <component v-bind:is="tabIcon(idx)"></component></span>
        </div>
      </div>
    </div>

    <!-- Dynamic component -->
    <component
      v-bind:is="currentTabComponent"
      v-bind:services="services"
      v-bind:leds="leds"
      v-bind:thingsboard="thingsboard"
      v-bind:schedule-config="schedule_config"
      v-bind:device-time="time"
      v-bind:device-cooling="cooling"
      v-bind:device-gpio="gpio"
    />
  </div>
</template>

<script>
import TabMain from './components/Main'
import TabMqtt from './components/Mqtt'
import TabTime from './components/Time'
import TabLeds from './components/Leds'
import TabFan from './components/Fan'
import TabGpio from './components/Gpio'
import TabUser from './components/User'
import { eventBus } from '@/eventBus'
import { getSettings } from '@/apiService'

export default {
  name: 'Settings',
  components: {
    TabMain,
    TabMqtt,
    TabTime,
    TabLeds,
    TabFan,
    TabGpio,
    TabUser
  },
  data () {
    return {
      currentTab: 'Main',
      tabs: ['Main', 'Time', 'MQTT', 'Leds', 'Fan', 'Gpio', 'User'],
      icons: ['settings-icon', 'clock-icon', 'share-2-icon', 'sun-icon', 'wind-icon', 'cpu-icon', 'user-icon'],
      services: {},
      leds: [],
      thingsboard: {},
      schedule_config: {},
      cooling: {},
      gpio: [],
      time: {}
    }
  },
  computed: {
    currentTabComponent () {
      return `tab-${this.currentTab.toLowerCase()}`
    }
  },
  mounted () {
    eventBus.$on('loadSettings', () => this.loadSettings())
    this.loadSettings()
  },
  destroyed () {
    eventBus.$off('loadSettings')
  },
  methods: {
    async loadSettings () {
      /* Load all settings */
      try {
        const response = await getSettings()
        this.services = response.data.services
        this.leds = response.data.leds
        this.time = response.data.time
        this.thingsboard = response.data.thingsboard
        this.schedule_config = response.data.schedule_config
        this.cooling = response.data.cooling
        this.gpio = response.data.gpio

        /* remove Fan tab if device not support it */
        if (!this.cooling) {
          this.tabs = this.tabs.filter(tab => tab !== 'Fan')
          this.icons = this.icons.filter(icon => icon !== 'wind-icon')
        }
      } catch (e) {
        if (e.response) {
          eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
    },
    selectTab (id) {
      this.currentTab = this.tabs[id]
    },
    tabIcon (id) {
      return this.icons[id]
    }
  }
}
</script>
