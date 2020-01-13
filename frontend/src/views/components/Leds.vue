<template>
  <div>
    <transition-group
      name="list"
      appear
    >
      <div
        v-for="(led, index) in leds"
        v-bind:key="led.id"
        class="columns is-marginless"
      >
        <div class="column">
          <div class="notification bg-notification is-light">
            <div class="field is-horizontal">
              <div class="field-label is-normal">
                <label class="label">Channel {{ led.id }}</label>
              </div>
              <div class="field-body">
                <!-- state -->
                <div class="field">
                  <div class="control has-text-centered">
                    <toggle-switch
                      v-model.number="led.state"
                      round
                    />
                  </div>
                  <p class="help">
                    Enable
                  </p>
                </div>
                <!-- color -->
                <div class="field">
                  <div class="control">
                    <div class="select">
                      <select
                        v-model="led.color"
                        name="color"
                      >
                        <option
                          v-for="(option, index) in $ledOptions"
                          v-bind:key="index"
                          v-bind:value="option.value"
                        >
                          {{ option.text }}
                        </option>
                      </select>
                    </div>
                    <p class="help">
                      Channel Color
                    </p>
                  </div>
                </div>
                <!-- power -->
                <div class="field">
                  <div class="control">
                    <input
                      v-model="led.power"
                      class="input"
                      type="text"
                      placeholder="channel power"
                    >
                  </div>
                  <p class="help">
                    Channel Power (Watts)
                  </p>
                </div>
              </div>
            </div>
          </div>
        </div>

        <div class="column">
          <div class="notification bg-notification has-text-dark has-text-left" v-bind:class="[index % 2 ? 'even' : 'odd']">
            <h4 class="subtitle">
              MQTT Topics
            </h4>

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
        </div>
      </div>
    </transition-group>

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
</template>

<script>
import { http } from '@/http'
import { eventBus } from '../../eventBus'

export default {
  name: 'Leds',
  data () {
    return {
      services: {},
      leds: []
    }
  },
  mounted () {
    this.loadLeds()
  },
  methods: {
    async saveLeds () {
      let response = await http.post('/api/settings', { leds: this.leds })
      if (response.data.success) {
        eventBus.$emit('message', 'Saved', 'success')
      } else {
        eventBus.$emit('message', 'NOT Saved', 'danger')
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

<style scoped lang="scss">
  $yellow: #ffdd57;
  .even {
    background-color: lighten($yellow, 20%);
  }
  .odd {
    background-color: lighten($yellow, 25%);
  }
</style>
