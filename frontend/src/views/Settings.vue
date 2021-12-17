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
        <div class="buttons has-addons">
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
    />
  </div>
</template>

<script>
import TabMain from './components/Main'
import TabMqtt from './components/Mqtt'
import TabTime from './components/Time'
import TabLeds from './components/Leds'
import TabFan from './components/Fan'
import TabUser from './components/User'
import { http } from '@/http'
import { eventBus } from '@/eventBus'

export default {
  name: 'Settings',
  components: {
    TabMain,
    TabMqtt,
    TabTime,
    TabLeds,
    TabFan,
    TabUser
  },
  data () {
    return {
      currentTab: 'Main',
      tabs: ['Main', 'Time', 'MQTT', 'Leds', 'Fan', 'User'],
      icons: ['settings-icon', 'clock-icon', 'share-2-icon', 'sun-icon', 'wind-icon', 'user-icon'],
      services: {},
      leds: [],
      thingsboard: {},
      schedule_config: {},
      cooling: {},
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
      let settingsResponse = await http.get('/api/settings')
      this.services = settingsResponse.data.services
      this.leds = settingsResponse.data.leds
      this.time = settingsResponse.data.time
      this.thingsboard = settingsResponse.data.thingsboard
      this.schedule_config = settingsResponse.data.schedule_config
      this.cooling = settingsResponse.data.cooling

      /* remove Fan tab if device not support it */
      if (!this.cooling) {
        this.tabs = this.tabs.filter(tab => tab !== 'Fan')
        this.icons = this.icons.filter(icon => icon !== 'wind-icon')
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
