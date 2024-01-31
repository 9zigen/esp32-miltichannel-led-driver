<template>
  <div class="container m-top-touch-navbar">
    <div class="columns is-marginless">
      <div class="column">
        <div class="content has-text-left">
          <h1 class="title">
            Schedule Settings
          </h1>
          <h3
            v-if="scheduleConfig.mode === 0"
            class="subtitle"
          >
            Sunrise -> Sunset
          </h3>
          <h3
            v-else
            class="subtitle"
          >
            Multipoint Mode
          </h3>
        </div>
      </div>
    </div>

    <!-- Chart -->
    <div
      v-if="series.length > 1"
      class="columns is-centered is-marginless"
    >
      <div class="column">
        <div class="box">
          <vue-chart
            id="schedule-chart"
            ref="scheduleChart"
            type="line"
            v-bind:labels="labels"
            v-bind:colors="colors"
            v-bind:line-options="{regionFill: 1}"
            v-bind:data-sets="series"
            v-bind:tooltip-options="tooltipOptions"
          />
        </div>
      </div>
    </div>

    <!-- Schedule Config: Simple Mode -->
    <div
      v-if="scheduleConfig.mode === 0"
      class="container"
    >
      <div class="columns is-centered is-marginless">
        <div class="column">
          <div class="box">
            <div class="columns is-centered is-mobile is-multiline">
              <!-- Sunrise -->
              <div class="column is-12-mobile is-2-tablet is-2-desktop">
                <div class="field">
                  <div class="control has-text-centered has-icons-left">
                    <input
                      class="input is-medium"
                      type="time"
                      pattern="[0-9]{2}:[0-9]{2}"
                      placeholder="00"
                      v-bind:value="timeToString(scheduleConfig.sunrise_hour, scheduleConfig.sunrise_minute)"
                      @blur="updateSunriseTime($event.target.value)"
                    >
                    <span class="icon is-medium is-left">
                      <sunrise-icon
                        size="1.5x"
                        class="m-t-12"
                      />
                    </span>
                  </div>
                </div>
              </div>

              <!-- Sunset -->
              <div class="column is-12-mobile is-2-tablet is-2-desktop">
                <div class="field">
                  <div class="control has-text-centered has-icons-left">
                    <input
                      class="input is-medium"
                      type="time"
                      pattern="[0-9]{2}:[0-9]{2}"
                      placeholder="00"
                      v-bind:value="timeToString(scheduleConfig.sunset_hour, scheduleConfig.sunset_minute)"
                      @blur="updateSunsetTime($event.target.value)"
                    >
                    <span class="icon is-medium is-left">
                      <sunset-icon
                        size="1.5x"
                        class="m-t-12"
                      />
                    </span>
                  </div>
                </div>
              </div>
            </div>

            <!-- Color picker -->
            <div
              v-if="scheduleConfig.rgb"
              class="columns is-centered is-mobile"
            >
              <div class="column is-flex is-narrow">
                <color-picker
                  v-bind="color"
                  @input="updateColorValue"
                />
              </div>
            </div>

            <!-- Brightness -->
            <div class="column">
              <div class="field is-horizontal">
                <div class="field-label">
                  <label class="label">Brightness</label>
                </div>
                <div class="field-body">
                  <div class="field">
                    <div class="control has-text-centered">
                      <slider
                        v-model.number="scheduleConfig.brightness"
                        min="0"
                        max="100"
                        @change="updateSimpleModeBrightness"
                      />
                    </div>
                  </div>
                </div>
              </div>

              <!-- Led channels -->
              <div
                v-for="(ledDuty, ledCh) in scheduleConfig.duty"
                v-show="active[ledCh]"
                v-bind:key="ledCh"
                class="field is-horizontal"
              >
                <div class="field-label">
                  <label class="label">CH {{ ledCh + 1 }}</label>
                </div>
                <div class="field-body">
                  <div class="field">
                    <div class="control has-text-centered">
                      <slider
                        v-model.number="scheduleConfig.duty[ledCh]"
                        min="0"
                        max="255"
                        v-bind:color="getChannelColor(ledCh)"
                        @change="updateColorPicker"
                      />
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <div
      v-else
      class="container"
    >
      <!-- Schedule Config: Multipoint Mode -->
      <div class="columns is-centered is-marginless">
        <div class="column">
          <div class="level">
            <div class="level-left" />
            <div class="level-right">
              <!-- Presets -->
              <div
                v-show="!is_empty"
                class="level-item"
              >
                <div class="field has-addons">
                  <div class="control">
                    <div class="select is-fullwidth">
                      <select
                        v-model="currentPreset"
                        name="preset"
                      >
                        <option v-bind:value="-1">
                          New Preset
                        </option>
                        <option
                          v-for="(preset, preset_id) in presets"
                          v-bind:key="preset_id"
                          v-bind:value="preset_id"
                        >
                          Preset {{ preset_id + 1 }}
                        </option>
                      </select>
                    </div>
                  </div>
                  <div class="control">
                    <a
                      class="button is-light is-success"
                      @click="applyPreset"
                    >Apply</a>
                  </div>
                  <div class="control">
                    <a
                      class="button is-light is-warning"
                      @click="savePreset"
                    >Save</a>
                  </div>
                </div>
              </div>
              <!-- Demo -->
              <div
                v-show="!is_empty"
                class="level-item"
              >
                <div class="field">
                  <div class="control">
                    <a
                      class="button is-link is-hidden-mobile"
                      @click="runDemo"
                    ><play-icon /> {{ run_demo_label }}</a>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <transition-group
            name="list"
            appear
          >
            <!-- Schedule -->
            <div
              v-for="(point, index) in schedule"
              v-bind:key="index"
              class="box"
            >
              <div class="columns is-marginless">
                <div class="column">
                  <div class="columns is-mobile is-multiline">
                    <!-- Time -->
                    <div class="column is-12-mobile is-2-tablet is-2-desktop">
                      <div class="field">
                        <div class="control has-text-centered">
                          <label
                            v-show="index === 0"
                            class="label is-hidden-mobile"
                          >HH:MM</label>
                          <label class="label is-hidden-tablet">HH:MM</label>
                          <input
                            class="input"
                            type="time"
                            pattern="[0-9]{2}:[0-9]{2}"
                            placeholder="00"
                            v-bind:value="timeToString(point.time_hour, point.time_minute)"
                            @blur="updateTime(index, $event.target.value)"
                          >
                        </div>
                        <p class="help is-hidden-mobile">
                          Time to apply
                        </p>
                      </div>
                    </div>

                    <div class="column is-12-mobile">
                      <!-- Brightness -->
                      <div class="field is-horizontal">
                        <div class="field-label">
                          <label class="label">Brightness</label>
                        </div>
                        <div class="field-body">
                          <div class="field">
                            <div class="control has-text-centered">
                              <slider
                                v-model.number="point.brightness"
                                min="0"
                                max="100"
                                @change="updateSeries"
                              />
                            </div>
                          </div>
                        </div>
                      </div>

                      <!-- Led channels -->
                      <div
                        v-for="(ledDuty, ledCh) in point.duty"
                        v-show="active[ledCh]"
                        v-bind:key="ledCh"
                        class="field is-horizontal"
                      >
                        <div class="field-label">
                          <label class="label">CH {{ ledCh + 1 }}</label>
                        </div>
                        <div class="field-body">
                          <div class="field">
                            <div class="control has-text-centered">
                              <slider
                                v-model.number="point.duty[ledCh]"
                                min="0"
                                max="255"
                                v-bind:color="getChannelColor(ledCh)"
                                @change="updateSeries"
                              />
                            </div>
                          </div>
                        </div>
                      </div>
                    </div>

                    <!-- Delete Mobile -->
                    <div class="column is-12 is-hidden-tablet">
                      <div class="field has-addons has-addons-centered">
                        <div class="control">
                          <span
                            class="button is-light"
                            @click="copyTimePoint(index)"
                          ><copy-icon class="mr-2"/> Copy</span>
                        </div>
                        <div class="control">
                          <span
                            class="button is-primary"
                            @click="applyTimePoint(index)"
                          ><check-icon class="mr-2"/>Now</span>
                        </div>
                        <div class="control">
                          <span
                            class="button is-danger"
                            @click="deleteTimePoint(index)"
                          ><x-icon class="mr-2"/> Delete</span>
                        </div>
                      </div>
                    </div>

                    <!-- Actions Desktop -->
                    <div class="column is-1 is-hidden-mobile is-multiline">
                      <div class="field">
                        <div class="control has-text-centered">
                          <span
                            class="button is-light"
                            @click="copyTimePoint(index)"
                          ><copy-icon /></span>
                        </div>
                        <div class="control has-text-centered mt-2">
                          <span
                            class="button is-primary"
                            @click="applyTimePoint(index)"
                          ><check-icon /></span>
                        </div>
                        <div class="control has-text-centered mt-2">
                          <span
                            class="button is-danger"
                            @click="deleteTimePoint(index)"
                          ><x-icon /></span>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
          </transition-group>
        </div>
      </div>
    </div>

    <!-- Buttons -->
    <div class="buttons is-centered">
      <span
        v-if="scheduleConfig.mode"
        class="button is-light"
        @click="addTimePoint"
      ><plus-icon />Add</span>
      <span
        class="button is-primary"
        @click="saveSchedule"
      ><check-icon /> Save and continue</span>
      <span
        v-if="scheduleConfig.mode"
        class="button is-ghost is-info"
        @click="runDemo"
      ><play-icon /> {{ run_demo_label }}</span>
      <span
        class="button is-danger"
        @click="reloadSchedule"
      ><x-icon /> Cancel</span>
    </div>
  </div>
</template>

<script>
import ColorPicker from '@radial-color-picker/vue-color-picker'
import { convertHslToRgb, convertRgbToHsl } from '@/components/Helpers/colorConvertion'
import { toMinutes, fromMinutes } from '@/components/Helpers/timeCalculation'
import { eventBus } from '../eventBus'
import { http } from '../http'

export default {
  name: 'Schedule',
  components: {
    ColorPicker
  },
  data () {
    return {
      active: [],
      colors: ['purple', '#ffa3ef', 'light-blue'],
      tooltipOptions: {
        formatTooltipX: d => (`${d}`).toUpperCase(),
        formatTooltipY: d => `${d}%`
      },
      labels: [],
      series: [],
      schedule: [],
      leds: [],
      capacity: 12,
      presets: [],
      currentPreset: -1,
      scheduleConfig: {
        mode: 1, /* 0 -> simple sunrise/sunset; 1 -> multiple points */
        rgb: false, /* RGB Picker, use first 3 channel as Red, Green, Blue */
        transition: 1800, /* simple mode transition in seconds */
        sunrise_hour: 10,
        sunrise_minute: 0,
        sunset_hour: 22,
        sunset_minute: 0,
        brightness: 100
      },
      color: {
        hue: 0,
        saturation: 100,
        luminosity: 50,
        alpha: 1
      },
      isDemo: false,
      demoHour: 0,
      debounceTimer: null,
      demoTimer: null
    }
  },
  computed: {
    run_demo_label: function () {
      let prefix = this.demoHour < 10 ? '0' : ''
      return !this.isDemo ? 'Run Demo' : prefix + this.demoHour + ':00 Stop'
    },
    is_empty: function () {
      return this.schedule.length === 0
    }
  },
  mounted () {
    (async () => {
      /* Light schedule */
      let scheduleResponse = await http.get('/api/schedule')
      this.schedule = scheduleResponse.data.schedule

      /* Led color data and schedule config */
      let settingsResponse = await http.get('/api/settings')
      this.leds = settingsResponse.data.leds

      /* set colors and active channels */
      if (this.leds.length > 0) {
        this.colors = this.leds.filter(led => led.state === 1).map((led) => led.color)
        this.active = this.leds.map((led) => led.state)
      }

      /* Schedule config */
      this.scheduleConfig = settingsResponse.data.schedule_config

      /* set color picker initial value */
      this.updateColorPicker()

      /* Update chart */
      this.graphSchedule()
      this.isLoading = false
    })()

    /* local presets */
    if (localStorage.getItem('presets')) {
      try {
        this.presets = JSON.parse(localStorage.getItem('presets'))
      } catch (e) {
        localStorage.removeItem('presets')
      }
    }
  },
  destroyed () {
    clearInterval(this.demoTimer)
  },
  methods: {
    asTime (value) {
      let hours = parseInt(value / 60)
      let minutes = value % 60
      if (hours < 10) {
        hours = `0${hours}`
      }
      if (minutes < 10) {
        minutes = `0${minutes}`
      }
      return `${hours}:${minutes}`
    },
    updateSeries () {
      /* simple mode */
      if (this.scheduleConfig.mode === 0) {
        let _schedule = []
        let sunrise = toMinutes(this.scheduleConfig.sunrise_hour, this.scheduleConfig.sunrise_minute)
        let sunset = toMinutes(this.scheduleConfig.sunset_hour, this.scheduleConfig.sunset_minute)
        const duty = this.leds.map((value) => 0)

        /* before sunrise */
        _schedule.push({
          enabled: true,
          time_hour: this.scheduleConfig.sunrise_hour,
          time_minute: this.scheduleConfig.sunrise_minute,
          brightness: 0,
          duty: [...duty]
        })

        /* sunrise point */
        _schedule.push({
          enabled: true,
          time_hour: fromMinutes(sunrise + this.scheduleConfig.simple_mode_duration).hour,
          time_minute: fromMinutes(sunrise + this.scheduleConfig.simple_mode_duration).minute,
          brightness: this.scheduleConfig.brightness,
          duty: [...this.scheduleConfig.duty]
        })

        /* before sunset point */
        _schedule.push({
          enabled: true,
          time_hour: fromMinutes(sunset).hour,
          time_minute: fromMinutes(sunset).minute,
          brightness: this.scheduleConfig.brightness,
          duty: [...this.scheduleConfig.duty]
        })

        /* sunset */
        _schedule.push({
          enabled: true,
          time_hour: fromMinutes(sunset + this.scheduleConfig.simple_mode_duration).hour,
          time_minute: fromMinutes(sunset + this.scheduleConfig.simple_mode_duration).minute,
          brightness: 0,
          duty: [...duty]
        })

        this.schedule = JSON.parse(JSON.stringify(_schedule))

        const _labels = _schedule.map(v => this.timeToString(v.time_hour, v.time_minute))
        const _series = this.leds.filter(led => led.state === 1).map((value, index) => ({
          name: `LED CH ${index + 1}`,
          values: _schedule.map((v) => Math.round(v.duty[index] * v.brightness / 255))
        }))

        this.labels = _labels
        this.series = _series

        setTimeout(() => {
          this.$refs.scheduleChart.update()
        }, 200)
      } else if (this.schedule.length > 0) {
        const _labels = this.schedule.map(v => this.timeToString(v.time_hour, v.time_minute))
        const _series = this.leds.filter(led => led.state === 1).map((value, index) => ({
          name: `LED CH ${index + 1}`,
          values: this.schedule.map((v) => Math.round(v.duty[index] * v.brightness / 255))
        }))

        this.labels = _labels
        this.series = _series

        setTimeout(() => {
          this.$refs.scheduleChart.update()
        }, 200)
      } else {
        this.series = []
      }
    },
    updateTime (index, value) {
      if (value !== '') {
        const re = /(?<hours>\d{2}):(?<minutes>\d{2})/
        const match = re.exec(value)

        if (match != null) {
          if (match.groups.hours && match.groups.minutes) {
            const hours = parseInt(match.groups.hours)
            if (hours > 24) {
              match.groups.hours = 24
            }

            let minutes = parseInt(match.groups.minutes)
            if (minutes > 60) {
              minutes = 60
            }

            this.schedule[index].time_hour = hours
            this.schedule[index].time_minute = minutes

            this.debounceUpdateSeries()
          }
        } else {
          this.schedule[index].time_hour = 0
          this.schedule[index].time_minute = 0
        }
      }
    },
    addTimePoint () {
      if (this.schedule.length < this.capacity) {
        /* use default duty from settings */
        const duty = this.leds.map((value) => 0)
        const lastSchedule = this.schedule[this.schedule.length - 1]

        this.schedule.push({
          enabled: true,
          time_hour: this.schedule.length ? lastSchedule.time_hour : 0,
          time_minute: this.schedule.length ? lastSchedule.time_minute + 30 : 1,
          brightness: 100,
          duty: [...duty]
        })
        this.updateSeries()
      } else {
        /* error - MAX Schedules reached */
        eventBus.$emit('message', 'MAX Schedules reached', 'danger')
      }
    },
    deleteTimePoint (id) {
      this.schedule.splice(id, 1)
      this.updateSeries()
    },
    copyTimePoint (id) {
      if (this.schedule.length < this.capacity) {
        let copySchedule = JSON.parse(JSON.stringify(this.schedule[id]))
        const sourceSchedule = this.schedule[id]

        copySchedule.time_hour = sourceSchedule.time_hour
        copySchedule.time_minute = sourceSchedule.time_minute + 30

        this.schedule.splice(id, 0, copySchedule)
        this.updateSeries()
      } else {
        /* error - MAX Schedules reached */
        eventBus.$emit('message', 'MAX Schedules reached', 'danger')
      }
    },
    async applyTimePoint (id) {
      if (this.schedule[id]) {
        let config = this.schedule[id]
        let response = await http.post('/api/light', {
          channels: config.duty,
          brightness: config.brightness })

        if (response.data.success) { eventBus.$emit('message', 'Applied...', 'success') }
      }
    },
    timeToString (hour, minute) {
      const _hour = hour < 10 ? `0${hour}` : hour
      const _minute = minute < 10 ? `0${minute}` : minute
      return `${_hour}:${_minute}`
    },
    graphSchedule () {
      /* refresh series */
      this.updateSeries()

      /* max capacity */
      if (this.schedule.capacity > 0) { this.capacity = this.schedule.capacity }

      /* Hide loader */
      eventBus.$emit('loading', false)
    },
    async reloadSchedule () {
      let response = await http.get('/api/schedule')
      this.schedule = response.data.schedule
    },
    async saveSchedule () {
      try {
        let response
        if (this.scheduleConfig.mode === 0) {
          response = await http.post('/api/settings', { schedule_config: this.scheduleConfig })
          response = await http.post('/api/schedule', { schedule: this.schedule })
        } else {
          response = await http.post('/api/schedule', { schedule: this.schedule })
        }
        if (response.data.success) {
          eventBus.$emit('message', 'Saved', 'success')
        }
      } catch (e) {
        if (e.response) {
          eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
    },
    getChannelColor (index) {
      return this.colors[index]
    },
    updateColorValue (hue) {
      let r, g, b
      [r, g, b] = convertHslToRgb(hue, 1, 0.50)

      /* Red */
      this.scheduleConfig.duty[0] = r
      /* Green */
      this.scheduleConfig.duty[1] = g
      /* Blue */
      this.scheduleConfig.duty[2] = b

      this.debounceUpdateSeries()
    },
    updateColorPicker () {
      let hue = convertRgbToHsl(this.scheduleConfig.duty[0], this.scheduleConfig.duty[1], this.scheduleConfig.duty[2])
      this.color = {
        hue: hue[0],
        saturation: 100,
        luminosity: 50,
        alpha: 1
      }
      this.debounceUpdateSeries()
    },
    updateSimpleModeBrightness (event) {
      /* Brightness */
      this.scheduleConfig.brightness = parseInt(event)
      this.debounceUpdateSeries()
    },
    debounceUpdateSeries () {
      if (this.debounceTimer === null) {
        this.debounceTimer = setTimeout(() => {
          this.updateSeries()
          this.debounceTimer = null
        }, 1000)
      }
    },
    updateSunriseTime (value) {
      if (value !== '') {
        const re = /(?<hours>\d{2}):(?<minutes>\d{2})/
        const match = re.exec(value)

        if (match.groups.hours && match.groups.minutes) {
          const hours = parseInt(match.groups.hours)
          if (hours > 24) {
            match.groups.hours = 24
          }

          let minutes = parseInt(match.groups.minutes)
          if (minutes > 60) {
            minutes = 60
          }

          this.scheduleConfig.sunrise_hour = hours
          this.scheduleConfig.sunrise_minute = minutes

          this.debounceUpdateSeries()
        }
      }
    },
    updateSunsetTime (value) {
      if (value !== '') {
        const re = /(?<hours>\d{2}):(?<minutes>\d{2})/
        const match = re.exec(value)

        if (match.groups.hours && match.groups.minutes) {
          const hours = parseInt(match.groups.hours)
          if (hours > 24) {
            match.groups.hours = 24
          }

          let minutes = parseInt(match.groups.minutes)
          if (minutes > 60) {
            minutes = 60
          }

          this.scheduleConfig.sunset_hour = hours
          this.scheduleConfig.sunset_minute = minutes

          this.debounceUpdateSeries()
        }
      }
    },
    async savePreset () {
      /* none selected */
      if (this.currentPreset === -1) {
        if (this.presets.length < 3) {
          /* add new item */
          this.presets.push(JSON.stringify(this.schedule))
          const parsed = JSON.stringify(this.presets)
          localStorage.setItem('presets', parsed)

          eventBus.$emit('message', 'Preset saved', 'success')
        } else {
          eventBus.$emit('message', 'Maximum number of presets reached', 'danger')
        }
      } else if (this.currentPreset > -1) {
        /* update item */
        this.presets[this.currentPreset] = JSON.stringify(this.schedule)
        const parsed = JSON.stringify(this.presets)
        localStorage.setItem('presets', parsed)

        eventBus.$emit('message', 'Preset updated', 'success')
      }
    },
    applyPreset () {
      /* none selected */
      if (this.currentPreset === -1) {
        eventBus.$emit('message', 'Select preset first', 'danger')
      } else if (this.currentPreset > -1) {
        this.schedule = JSON.parse(this.presets[this.currentPreset])
        this.debounceUpdateSeries()
        eventBus.$emit('message', 'Preset applied...', 'success')
      }
    },
    async runDemo () {
      this.isDemo = !this.isDemo
      if (this.isDemo) {
        this.demoTimer = setInterval(() => {
          this.schedule.forEach((item, id) => {
            if (item.time_hour === this.demoHour) {
              http.post('/api/light', {
                channels: item.duty,
                brightness: item.brightness })
            }
          })
          this.demoHour === 23 ? this.demoHour = 0 : this.demoHour++
        }, 2000)
      } else {
        clearInterval(this.demoTimer)
        this.demoHour = 0
      }
    }
  }
}
</script>
