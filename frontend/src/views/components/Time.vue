<template>
  <div class="container">
    <div class="tile is-ancestor is-marginless">
      <div class="tile is-vertical">
        <!-- top row -->
        <div class="tile">
          <!-- left col -->
          <div class="tile is-vertical is-4">
            <div class="tile is-parent">
              <article class="tile is-child has-text-left">
                <span class="subtitle is-4 is-uppercase has-text-grey-light">Local Time</span>
                <div class="box has-text-centered">
                  <div class="content">
                    <clock v-bind:time="timeString" />
                  </div>
                </div>
              </article>
            </div>
          </div>

          <!-- right col -->
          <div
            v-if="deviceTime"
            class="tile is-vertical"
          >
            <div class="tile is-parent">
              <article class="tile is-child has-text-left">
                <span class="subtitle is-4 is-uppercase has-text-grey-light">RTC Time</span>
                <div class="box">
                  <!-- Hours -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Hours</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <input-number
                            v-model.number="deviceTime.hour"
                            class="input"
                            maxlength="2"
                            placeholder="hour"
                          />
                        </div>
                      </div>
                    </div>
                  </div>

                  <!-- Minutes -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Minutes</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <input-number
                            v-model.number="deviceTime.minute"
                            class="input"
                            maxlength="2"
                            placeholder="minutes"
                          />
                        </div>
                      </div>
                    </div>
                  </div>

                  <!-- Seconds -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Seconds</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <input-number
                            v-model.number="deviceTime.second"
                            class="input"
                            maxlength="2"
                            placeholder="second"
                          />
                        </div>
                      </div>
                    </div>
                  </div>
                </div>

                <span class="subtitle is-4 is-uppercase has-text-grey-light">RTC Date</span>
                <div class="box">
                  <!-- Year -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Year</label>
                    </div>
                    <div class="field-body">
                      <div class="field has-addons">
                        <div class="control">
                          <a class="button is-static">
                            20
                          </a>
                        </div>
                        <div class="control is-expanded">
                          <input-number
                            v-model.number="deviceTime.year"
                            class="input"
                            maxlength="2"
                            placeholder="year"
                          />
                        </div>
                      </div>
                    </div>
                  </div>
                  <!-- Month -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Month</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <input-number
                            v-model.number="deviceTime.month"
                            class="input"
                            maxlength="2"
                            placeholder="month"
                          />
                        </div>
                      </div>
                    </div>
                  </div>
                  <!-- Day -->
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Day</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <input-number
                            v-model.number="deviceTime.day"
                            class="input"
                            maxlength="2"
                            placeholder="day"
                          />
                        </div>
                      </div>
                      <div class="field">
                        <div class="control">
                          <div class="select">
                            <select
                              v-model.number="deviceTime.weekday"
                              name="weekday"
                            >
                              <option
                                v-for="(option, index) in weekdayOptions"
                                v-bind:key="index"
                                v-bind:value="option.value"
                              >
                                {{ option.text }}
                              </option>
                            </select>
                          </div>
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </article>
            </div>
            <div class="tile is-parent">
              <article class="tile is-child has-text-left">
                <span class="subtitle is-4 is-uppercase has-text-grey-light">NTP</span>
                <div class="box">
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Server Name</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control is-fullwidth">
                          <input
                            v-model="services.ntp_server"
                            class="input"
                            type="text"
                            placeholder="NTP Server Name"
                          >
                        </div>
                      </div>
                    </div>
                  </div>
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">Enable</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <toggle-switch
                            v-model="services.enable_ntp"
                            round
                          />
                        </div>
                      </div>
                    </div>
                  </div>
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">TimeZone</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <div class="select is-fullwidth">
                            <select
                              v-model.number="services.utc_offset"
                            >
                              <option
                                v-for="(option, index) in $tzOptions"
                                v-bind:key="index"
                                v-bind:value="option.value"
                              >
                                {{ option.text }} ({{ option.value }})
                              </option>
                            </select>
                          </div>
                        </div>
                      </div>
                    </div>
                  </div>
                  <div class="field is-horizontal">
                    <div class="field-label is-normal">
                      <label class="label">DST</label>
                    </div>
                    <div class="field-body">
                      <div class="field">
                        <div class="control">
                          <toggle-switch
                            v-model="services.ntp_dst"
                            round
                          />
                        </div>
                      </div>
                    </div>
                  </div>
                </div>
              </article>
            </div>
          </div>
        </div>

        <!-- bottom row -->
        <div class="tile">
          <div class="tile is-vertical">
            <div class="tile is-parent">
              <div class="container">
                <div class="buttons is-centered">
                  <span
                    class="button is-primary"
                    @click="saveTime"
                  >
                    <check-icon
                      size="1.5x"
                      class="custom-class"
                    /> Apply
                  </span>
                  <span
                    class="button is-danger"
                    @click="loadTime"
                  >
                    <x-icon
                      size="1.5x"
                      class="custom-class"
                    /> Cancel
                  </span>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
import { eventBus } from '@/eventBus'
import { http } from '@/http'
import Clock from '../../components/Ui/Clock'

export default {
  name: 'Time',
  components: { Clock },
  props: {
    deviceTime: {
      type: Object,
      required: true
    },
    services: {
      type: Object,
      required: true
    }
  },
  data () {
    return {
      timeString: '',
      browserTime: {
        hour: '',
        minute: '',
        second: ''
      },
      date: new Date(),
      dstOptions: [
        { text: 'No ', value: '0' },
        { text: 'Yes', value: '1' }
      ],
      weekdayOptions: [
        { text: 'Monday', value: '1' },
        { text: 'Tuesday', value: '2' },
        { text: 'Wednesday', value: '3' },
        { text: 'Thursday', value: '4' },
        { text: 'Friday', value: '5' },
        { text: 'Saturday', value: '6' },
        { text: 'Sunday', value: '7' }
      ],
      refreshInterval: null
    }
  },
  mounted () {
    // this.loadTime()

    /* update current time */
    this.refreshInterval = setInterval(() => {
      let dateTime = new Date()
      this.browserTime.hour = dateTime.getHours()
      this.browserTime.minute = dateTime.getMinutes()
      this.browserTime.second = dateTime.getSeconds()
      this.timeString = this.timeToString()
    }, 1000)
  },
  destroyed () {
    clearInterval(this.refreshInterval)
  },
  methods: {
    async rebootDevice () {
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
    },
    async saveTime () {
      try {
        let response = await http.post('/api/settings', {
          time: this.deviceTime,
          services: this.services
        })
        if (response.data.success) {
          eventBus.$emit('message', 'Saved', 'success')
          setTimeout(() => {
            if (confirm('Reboot Led Controller to apply the time settings?')) {
              this.rebootDevice()
            }
          }, 3000)
        }
      } catch (e) {
        if (e.response) {
          eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
    },
    async loadTime () {
      eventBus.$emit('loadSettings')
    },
    timeToString () {
      return this.browserTime.hour + ':' + this.browserTime.minute + ':' + this.browserTime.second
    }
  }
}
</script>
