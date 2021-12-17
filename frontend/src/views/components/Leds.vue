<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12 has-text-left">
      <!-- LEDS -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Led channels</span>
      <div
        v-for="(led, index) in leds"
        v-bind:key="led.id"
        class="has-text-left"
      >
        <div class="box mb-2">
          <!-- Header -->
          <div class="box-header">
            <div class="level is-mobile">
              <div class="level-left">
                <div class="level-item">
                  <h3 class="title is-5 is-marginless has-text-grey">
                    Channel #{{ led.id }}
                  </h3>
                </div>
              </div>
              <div class="level-right">
                <div class="level-item">
                  <toggle-switch
                    v-model.number="led.state"
                    round
                  />
                </div>
              </div>
            </div>
          </div>

          <!-- Color -->
          <div class="field is-horizontal">
            <div class="field-label is-normal">
              <label class="label">Color</label>
            </div>
            <div class="field-body">
              <div class="field is-narrow">
                <div class="control">
                  <div class="select is-fullwidth">
                    <select
                      v-model="led.color"
                      name="color"
                    >
                      <option
                        v-for="(color, color_id) in $ledColorOptions"
                        v-bind:key="color_id"
                        v-bind:value="color.value"
                      >
                        {{ color.text }}
                      </option>
                    </select>
                  </div>
                </div>
              </div>
            </div>
          </div>

          <!-- duty limit -->
          <div class="field is-horizontal">
            <div class="field-label is-normal">
              <label class="label">Duty max</label>
            </div>
            <div class="field-body">
              <div class="field has-addons is-narrow">
                <div class="control">
                  <input
                    v-model.number="led.duty_max"
                    class="input"
                    type="text"
                    placeholder="Duty limit"
                  >
                </div>
                <p class="control">
                  <a class="button is-static">
                    0 - 255
                  </a>
                </p>
              </div>
            </div>
          </div>

          <!-- power -->
          <div class="field is-horizontal">
            <div class="field-label is-normal">
              <label class="label">Power</label>
            </div>
            <div class="field-body">
              <div class="field has-addons is-narrow">
                <div class="control">
                  <input
                    v-model.number="led.power"
                    class="input"
                    type="text"
                    placeholder="channel power"
                  >
                </div>
                <p class="control">
                  <a class="button is-static">
                    Watts
                  </a>
                </p>
              </div>
            </div>
          </div>

          <div>
            <a
              v-show="!mqtt_info[index]"
              class="m-b-10"
              @click="showDetails(index)"
            >
              show mqtt topics
            </a>

            <a
              v-show="mqtt_info[index]"
              @click="showDetails(index)"
            >
              hide mqtt topics
            </a>

            <transition name="fade">
              <div v-show="mqtt_info[index]">
                <ul class="m-b-10">
                  <li>On/Off GET {{ services.hostname }}/channel/{{ led.id }}/state</li>
                  <li>On/Off SET {{ services.hostname }}/channel/{{ led.id }}/switch</li>
                </ul>

                <ul class="m-b-10">
                  <li>Brightness GET {{ services.hostname }}/brightness</li>
                  <li>Brightness SET {{ services.hostname }}/brightness/set</li>
                </ul>

                <ul>
                  <li>Channel Duty in % GET {{ services.hostname }}/channel/{{ led.id }}</li>
                  <li>Channel Duty in % SET {{ services.hostname }}/channel/{{ led.id }}/set</li>
                </ul>
              </div>
            </transition>
          </div>
        </div>
      </div>

      <!-- Buttons -->
      <div class="buttons is-centered">
        <span
          class="button is-primary"
          @click="saveLeds"
        >
          <check-icon size="1.5x" /> Apply
        </span>

        <span
          class="button is-danger"
          @click="loadLeds"
        >
          <x-icon size="1.5x" /> Cancel
        </span>
      </div>
    </div>
  </div>
</template>

<script>
import { http } from '@/http'
import { eventBus } from '../../eventBus'

export default {
  name: 'Leds',
  data () {
    return {
      services: {},
      leds: [],
      schedule_mode: 0,
      mqtt_info: []
    }
  },
  mounted () {
    this.loadLeds()
    this.mqtt_info = new Array(8).fill(0, 0, 7)
  },
  methods: {
    showDetails (id) {
      if (this.mqtt_info[id] === 0) {
        this.mqtt_info.splice(id, 1, 1)
      } else {
        this.mqtt_info.splice(id, 1, 0)
      }
    },
    async saveLeds () {
      try {
        let response = await http.post('/api/settings', { leds: this.leds })
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
    async loadLeds () {
      let response = await http.get('/api/settings')
      this.leds = response.data.leds
      this.services = response.data.services
    }
  }
}
</script>
