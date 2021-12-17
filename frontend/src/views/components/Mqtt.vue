<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12 has-text-left">
      <!-- MQTT -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">MQTT</span>
      <div class="box">
        <!-- Server IP -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Server IP Address</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input-ip
                  v-model="services.mqtt_ip_address"
                  placeholder="IP Address"
                />
              </div>
            </div>
          </div>
        </div>
        <!-- Port -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Server Port</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input-number
                  v-model="services.mqtt_port"
                  class="input"
                  type="number"
                  placeholder="Server Port"
                />
              </div>
            </div>
          </div>
        </div>
        <!-- User -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">User</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="services.mqtt_user"
                  class="input"
                  type="text"
                  placeholder="User"
                >
              </div>
            </div>
          </div>
        </div>
        <!-- User -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Password</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="services.mqtt_password"
                  class="input"
                  type="password"
                  placeholder="Password"
                >
              </div>
            </div>
          </div>
        </div>
        <!-- Enable -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">MQTT</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <toggle-switch
                  v-model="services.enable_mqtt"
                  round
                />
              </div>
              <p class="help">
                Enable
              </p>
            </div>
          </div>
        </div>
        <!-- QoS -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">QoS</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <div class="select">
                  <select
                    v-model.number="services.mqtt_qos"
                    name="mqtt_qos"
                  >
                    <option
                      v-for="(option, index) in $mqttOptions"
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

      <!-- ThingsBoard -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">Things Board</span>
      <div class="box">
        <!-- Endpoint -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Endpoint</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="thingsboard.endpoint"
                  class="input"
                  type="text"
                  placeholder="endpoint url"
                >
              </div>
            </div>
          </div>
        </div>
        <!-- Token -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Token</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <input
                  v-model="thingsboard.token"
                  class="input"
                  type="text"
                  placeholder="device token"
                >
              </div>
            </div>
          </div>
        </div>
        <!-- Enable -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Enable</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <toggle-switch
                  v-model="thingsboard.enable"
                  round
                />
              </div>
            </div>
          </div>
        </div>
        <!-- RPC -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">RPC</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <toggle-switch
                  v-model="thingsboard.rpc"
                  round
                />
              </div>
            </div>
          </div>
        </div>
        <!-- MQTT QoS -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">MQTT QoS</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <div class="select">
                  <select
                    v-model.number="thingsboard.qos"
                    name="mqtt_qos"
                  >
                    <option
                      v-for="(option, index) in $mqttOptions"
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
        <!-- MQTT Retain -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">MQTT Retain</label>
          </div>
          <div class="field-body">
            <div class="field">
              <div class="control">
                <toggle-switch
                  v-model="thingsboard.retain"
                  round
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
          @click="saveMqtt"
        >
          <check-icon
            size="1.5x"
            class="custom-class"
          /> Apply
        </span>
        <span
          class="button is-danger"
          @click="loadMqtt"
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
  name: 'Mqtt',
  props: {
    thingsboard: {
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
    this.loadMqtt()
  },
  methods: {
    async saveMqtt () {
      try {
        let response = await http.post('/api/settings', {
          thingsboard: this.thingsboard,
          services: this.services
        })
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
    async loadMqtt () {
      eventBus.$emit('loadSettings')
    }
  }
}
</script>
