<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12 has-text-left">
      <!-- Fan -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Fan Controller</span>
      <div class="box">
        <!-- Installed -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Installed</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <toggle-switch
                  v-model="deviceCooling.installed"
                  round
                />
              </div>
            </div>
          </div>
        </div>

        <!-- Start Temperature -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Start Temperature</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.start_temp"
                  class="input"
                  maxlength="2"
                  placeholder="Start Temperature"
                />
              </div>
              <p class="control">
                <a class="button is-static">
                  °C
                </a>
              </p>
            </div>
          </div>
        </div>

        <!-- Target Temperature -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Target Temperature</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.target_temp"
                  class="input"
                  maxlength="2"
                  placeholder="Target Temperature"
                />
              </div>
              <p class="control">
                <a class="button is-static">
                  °C
                </a>
              </p>
            </div>
          </div>
        </div>

        <!-- Max Temperature -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Max Temperature</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.max_temp"
                  class="input"
                  maxlength="2"
                  placeholder="Target Temperature"
                />
              </div>
              <p class="control">
                <a class="button is-static">
                  °C
                </a>
              </p>
            </div>
          </div>
        </div>

        <!-- Tahometer -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">FAN speed</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <span class="button is-static">{{ deviceCooling.tachometer }}</span>
              </div>
              <div class="control">
                <span class="button is-static">rpm</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <!-- Fan PID -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Fan PID</span>
      <div class="box">
        <!-- Proportional gain -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Proportional gain</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.pid_kp"
                  class="input"
                  maxlength="3"
                  step="0.1"
                  placeholder="proportional gain"
                />
              </div>
            </div>
          </div>
        </div>
        <!-- integral gain -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Integral gain</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.pid_ki"
                  class="input"
                  maxlength="3"
                  step="0.1"
                  placeholder="integral gain"
                />
              </div>
            </div>
          </div>
        </div>
        <!-- integral gain -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Derivative gain</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input-number
                  v-model.number="deviceCooling.pid_kd"
                  class="input"
                  maxlength="3"
                  step="0.1"
                  placeholder="derivative gain"
                />
              </div>
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
          @click="saveFan"
        >
          <check-icon
            size="1.5x"
            class="custom-class"
          /> Apply
        </span>
        <span
          class="button is-danger"
          @click="loadFan"
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
  name: 'Fan',
  props: {
    deviceCooling: {
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
    }
  },
  mounted () {
    this.loadFan()
  },
  methods: {
    async saveFan () {
      try {
        let response = await http.post('/api/settings', { cooling: this.deviceCooling })
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
    async loadFan () {
      eventBus.$emit('loadSettings')
    }
  }
}
</script>
