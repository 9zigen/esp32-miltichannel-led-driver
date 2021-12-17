<template>
  <div class="columns is-marginless is-multiline">
    <div class="column is-12 has-text-left">
      <!-- User -->
      <span class="subtitle is-4 is-uppercase has-text-grey-light">User</span>
      <div class="box">
        <!-- Name -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Name</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <input
                  v-model="auth.user"
                  class="input"
                  type="text"
                  placeholder="Name"
                />
              </div>
            </div>
          </div>
        </div>

        <!-- Password -->
        <div class="field is-horizontal">
          <div class="field-label is-normal">
            <label class="label">Password</label>
          </div>
          <div class="field-body">
            <div class="field has-addons">
              <div class="control">
                <input
                  v-model="auth.password"
                  class="input"
                  placeholder="Password"
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
          @click="saveUser"
        >
          <check-icon
            size="1.5x"
            class="custom-class"
          /> Apply
        </span>
        <span
          class="button is-danger"
          @click="loadUser"
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
import { getUser, setUser } from '@/apiService'

export default {
  name: 'User',
  data () {
    return {
      auth: {}
    }
  },
  mounted () {
    this.loadUser()
  },
  methods: {
    async saveUser () {
      try {
        let response = await setUser(this.auth)
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
    async loadUser () {
      try {
        let response = await getUser()
        if (response.data.auth) {
          this.auth = response.data.auth
        }
      } catch (e) {
        if (e.response) {
          eventBus.$emit('message', e.response.data.message, 'danger')
        } else {
          eventBus.$emit('message', 'unexpected error', 'danger')
        }
      }
    }
  }
}
</script>
