<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12 has-text-left">
      <!-- Gpio main function -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Gpio Main Functions</span>
      <div class="box">
        <div class="columns is-multiline">
          <div
            v-for="(gpio) in deviceGpio"
            v-bind:key="gpio.pin"
            class="column is-12"
          >
            <!-- Gpio PIN -->
            <div class="field is-horizontal">
              <div class="field-label is-normal">
                <label class="label">PIN {{gpio.pin}}</label>
              </div>
              <div class="field-body">
                <div class="field">
                  <div class="control">
                    <div class="select">
                      <select
                        v-model.number="gpio.function"
                        name="pin"
                      >
                        <option
                          v-for="(option, index) in gpioFunctions"
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
        </div>
      </div>
      <!-- Gpio alternative function -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Gpio Alternative Functions</span>
      <div class="box">
        <div class="columns is-multiline">
          <div
            v-for="(gpio) in deviceGpio"
            v-bind:key="gpio.pin"
            class="column is-12"
          >
            <!-- Gpio PIN -->
            <div class="field is-horizontal">
              <div class="field-label is-normal">
                <label class="label">PIN {{gpio.pin}}</label>
              </div>
              <div class="field-body">
                <div class="field">
                  <div class="control">
                    <div class="select">
                      <select
                        v-model.number="gpio.alt_function"
                        name="pin"
                      >
                        <option
                          v-for="(option, index) in gpioFunctions"
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
        </div>
      </div>
    </div>

    <!-- Buttons -->
    <div class="column">
      <div class="buttons is-centered">
        <span
          class="button is-primary"
          @click="saveGpioConfig"
        >
          <check-icon
            size="1.5x"
            class="custom-class"
          /> Apply
        </span>
        <span
          class="button is-danger"
          @click="reset"
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
import { getGpio } from '@/apiService'

export default {
  name: 'Gpio',
  props: {
    deviceGpio: {
      type: Array,
      required: true
    }
  },
  data () {
    return {
      gpioFunctions: []
    }
  },
  mounted () {
    this.loadGpioOptions()
  },
  methods: {
    async loadGpioOptions () {
      try {
        let response = await getGpio()
        if (response.data.gpioFunctions) {
          this.gpioFunctions = response.data.gpioFunctions
        }
      } catch (e) {
        if (e.response) {
          eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
    },
    async saveGpioConfig () {
      try {
        let response = await http.post('/api/settings', { gpio: this.deviceGpio })
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
    async reset () {
      eventBus.$emit('loadSettings')
    }
  }
}
</script>
