<template>
  <div class="container">
    <div class="tile is-ancestor is-marginless">
      <div class="tile is-vertical">
        <!-- top row -->
        <div class="tile">
          <!-- left col -->
          <div class="tile is-vertical is-4">
            <div class="tile is-parent">
              <article class="tile is-child notification bg-notification is-light">
                <div class="content">
                  <p class="subtitle">
                    Local Time
                  </p>
                  <div class="content">
                    <clock v-bind:time="timeString" />
                  </div>
                </div>
              </article>
            </div>
          </div>

          <!-- right col -->
          <div class="tile is-vertical">
            <div class="tile is-parent">
              <article class="tile is-child notification bg-notification is-light">
                <h5 class="subtitle">
                  Device UTC Time
                </h5>
                <div class="field is-horizontal">
                  <div class="field-label is-normal">
                    <label class="label">Date</label>
                  </div>
                  <div class="field-body">
                    <div class="field">
                      <div class="control">
                        <input-number
                          v-model.number="deviceTime.year"
                          class="input"
                          maxlength="2"
                          placeholder="year"
                        />
                      </div>
                      <p class="help">
                        Year
                      </p>
                    </div>
                    <div class="field">
                      <div class="control">
                        <input-number
                            v-model.number="deviceTime.month"
                            class="input"
                            maxlength="2"
                            placeholder="month"
                        />
                      </div>
                      <p class="help">
                        Month
                      </p>
                    </div>
                    <div class="field">
                      <div class="control">
                        <input-number
                            v-model.number="deviceTime.day"
                            class="input"
                            maxlength="2"
                            placeholder="day"
                        />
                      </div>
                      <p class="help">
                        Day
                      </p>
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
                      <p class="help">
                        Weekday
                      </p>
                    </div>
                  </div>
                </div>

                <div class="field is-horizontal">
                  <div class="field-label is-normal">
                    <label class="label">Time</label>
                  </div>
                  <div class="field-body">
                    <!-- Hours -->
                    <div class="field">
                      <div class="control">
                        <input-number
                            v-model.number="deviceTime.hour"
                            class="input"
                            maxlength="2"
                            placeholder="hour"
                        />
                      </div>
                      <p class="help">
                        Hours
                      </p>
                    </div>

                    <!-- Minutes -->
                    <div class="field">
                      <div class="control">
                        <input-number
                            v-model.number="deviceTime.minute"
                            class="input"
                            maxlength="2"
                            placeholder="minutes"
                        />
                      </div>
                      <p class="help">
                        Minutes
                      </p>
                    </div>

                    <!-- Seconds -->
                    <div class="field">
                      <div class="control">
                        <input-number
                            v-model.number="deviceTime.second"
                            class="input"
                            maxlength="2"
                            placeholder="second"
                        />
                      </div>
                      <p class="help">
                        Seconds
                      </p>
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
              <article class="tile is-child notification bg-notification is-light">
                <div class="content">
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
              </article>
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
  data () {
    return {
      timeString: '',
      browserTime: {
        hour: '',
        minute: '',
        second: ''
      },
      deviceTime: {
        year: 20,
        month: 10,
        weekday: 1,
        day: 10,
        hour: 12,
        minute: 1,
        second: 1
      },
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
      ]
    }
  },
  mounted () {
    this.loadTime()

    /* update current time */
    setInterval(() => {
      let dateTime = new Date()
      this.browserTime.hour = dateTime.getHours()
      this.browserTime.minute = dateTime.getMinutes()
      this.browserTime.second = dateTime.getSeconds()
      this.timeString = this.timeToString()
    }, 1000)
  },
  methods: {
    async saveTime () {
      let response = await http.post('/api/settings', { time: this.deviceTime })
      if (response.data.success) {
        eventBus.$emit('message', 'Saved', 'success')
      } else {
        eventBus.$emit('message', 'NOT Saved', 'danger')
      }
    },
    async loadTime () {
      let response = await http.get('/api/settings')
      this.deviceTime = response.data.time
    },
    timeToString () {
      return this.browserTime.hour + ':' + this.browserTime.minute + ':' + this.browserTime.second
    }
  }
}
</script>
