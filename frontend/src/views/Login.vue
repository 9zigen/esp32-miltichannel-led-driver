<template>
  <section class="hero is-fullheight-with-navbar">
    <div class="hero-body">
      <div class="container">
        <h1 class="title">
          Login Form
        </h1>

        <div class="columns is-centered has-text-left">
          <div class="column is-4">
            <div class="box">
              <div class="field">
                <div class="field-body">
                  <div class="field">
                    <div class="control">
                      <input
                        v-model="name"
                        class="input"
                        type="text"
                        placeholder="User Name"
                      >
                    </div>
                  </div>
                </div>
              </div>
              <div class="field">
                <div class="field-body">
                  <div class="field">
                    <div class="control">
                      <input
                        v-model="password"
                        class="input"
                        type="password"
                        placeholder="Password"
                      >
                    </div>
                  </div>
                </div>
              </div>
            </div>

            <!-- Buttons -->
            <div class="buttons is-centered">
              <a
                class="button is-primary is-fullwidth"
                @click="authProcess"
              ><log-in-icon
                size="1.5x"
                class="p-1"
              /> Login</a>
            </div>
          </div>
        </div>
      </div>
    </div>
  </section>
</template>

<script>

import { authService } from '@/authService'
import { eventBus } from '@/eventBus'
import { http } from '@/http'

export default {
  name: 'Login',
  data () {
    return {
      name: null,
      password: null
    }
  },
  mounted () {
    this.checkConnection()
  },
  methods: {
    async authProcess () {
      try {
        let response = await authService({ user: this.name, password: this.password })
        if (response.data.success && response.data.token) {
          eventBus.$emit('message', 'Success', 'success')
          setTimeout(() => {
            this.$router.push('/')
          }, 500)
        }
      } catch (e) {
        eventBus.$emit('message', 'Error: username or password invalid', 'danger')
      }
    },
    async checkConnection () {
      eventBus.$emit('loading', true)
      try {
        /* Device status */
        let response = await http.get('/status')
        if (response.data.status !== undefined) {
          eventBus.$emit('loading', false)
        }
        // this.status = statusResponse.data.status
      } catch (e) {
        if (e.response) {
          eventBus.$emit('loading', false)
        } else {
          eventBus.$emit('message', 'Device offline. Check wifi connection', 'danger')
          eventBus.$emit('loading', true)
        }
      }
    }
  }
}
</script>
