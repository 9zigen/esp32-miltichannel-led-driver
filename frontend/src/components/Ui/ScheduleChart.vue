<template>
  <div>
    <vue-chart
      id="home-chart"
      type="line"
      v-bind:labels="labels"
      v-bind:colors="colors"
      v-bind:line-options="{regionFill: 1}"
      v-bind:data-sets="series"
      v-bind:tooltip-options="tooltipOptions"
    />
    <div
      v-show="!series.length"
      class="no-data"
    >
      Not set, <router-link to="/schedule">
        add
      </router-link>
    </div>
  </div>
</template>

<script>

import { eventBus } from '@/eventBus'
import { http } from '../../http'

export default {
  name: 'ScheduleChart',
  data () {
    return {
      colors: ['purple', '#ffa3ef', 'light-blue'],
      tooltipOptions: {
        formatTooltipX: d => (`${d}`).toUpperCase(),
        formatTooltipY: d => `${d}%`
      },
      labels: [],
      series: [],
      schedule: [],
      scheduleConfig: {}
    }
  },
  mounted () {
    this.requestData()
  },
  methods: {
    async requestData () {
      eventBus.$emit('loading', true)
      try {
        /* Led color data and schedule config */
        let settingsResponse = await http.get('/api/settings')
        this.leds = settingsResponse.data.leds

        /* Light schedule */
        let scheduleResponse = await http.get('/api/schedule')
        this.schedule = scheduleResponse.data.schedule

        /* Schedule config */
        this.scheduleConfig = settingsResponse.data.schedule_config

        this.drawSchedule()
      } catch (e) {
        eventBus.$emit('message', e, 'danger')
      }
      eventBus.$emit('loading', false)
    },
    timeToString (hour, minute) {
      const _hour = hour < 10 ? `0${hour}` : hour
      const _minute = minute < 10 ? `0${minute}` : minute
      return `${_hour}:${_minute}`
    },
    async drawSchedule () {
      /* set colors */
      if (this.leds) {
        this.colors = this.leds.filter(led => led.state === 1).map((value, index, array) => value.color)
      }

      if (this.schedule.length > 0) {
        if (this.scheduleConfig.mode === 1) {
          /* Sort array by TIME */
          this.schedule.sort((a, b) => (a.time_hour * 60 + a.time_minute) - (b.time_hour * 60 + b.time_minute))
        }

        const _labels = this.schedule.map(v => this.timeToString(v.time_hour, v.time_minute))
        const _series = this.leds.filter(led => led.state === 1).map((value, index) => ({
          name: `CH ${index + 1}`,
          values: this.schedule.map((v) => Math.round(v.duty[index] * v.brightness / 255))
        }))

        this.labels = _labels
        this.series = _series
      }
    }
  }

}
</script>
