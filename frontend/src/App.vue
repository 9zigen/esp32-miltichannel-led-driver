<template>
  <div
    id="app"
    class="has-background-grey-dark has-text-light"
  >
    <navbar />
    <router-view v-bind:class="{'is-disabled': isLoading}" />
    <notification
      v-bind:message="notify.text"
      v-bind:type="notify.type"
      v-bind:show-message="notify.visible"
    />
    <div
      v-show="isLoading"
      class="loader"
    />
  </div>
</template>

<script>
import Notification from '@/components/Ui/Notification'
import Navbar from '@/components/Ui/Navbar'
import { eventBus } from './eventBus'
import { mutations } from '@/store'
import { http } from '@/http'

export default {
  name: 'App',
  components: {
    Navbar, Notification
  },
  data () {
    return {
      ws: {
        url: '',
        connected: false
      },
      isLoading: false,
      notify: {
        type: 'success',
        text: 'text',
        visible: false,
        autoHide: null,
        timeOut: 2000
      }
    }
  },
  created: function () {
    let vm = this
    http.interceptors.response.use(undefined, function (err) {
      return new Promise(function (resolve, reject) {
        if (err.response.status === 401 && err.response.config && !err.response.config.__isRetryRequest) {
          // if you ever get an unauthorized, logout the user
          mutations.logout()
          if (vm.$route.name !== 'login') {
            vm.$router.push('/login')
          }
        }
        throw err
      })
    })
  },
  mounted () {
    eventBus.$on('message', (text, type) => {
      this.showMessage(text, type)
    })

    eventBus.$on('loading', (show) => {
      this.isLoading = !!show
    })
  },
  destroyed () {
    if (this.notify.autoHide) clearInterval(this.autoHide)
    eventBus.$off()
  },
  methods: {
    hideMessage () {
      this.notify.visible = false
    },
    showMessage (text, type) {
      this.notify.text = text
      this.notify.type = type
      this.notify.visible = true
      clearTimeout(this.notify.autoHide)
      this.notify.autoHide = setTimeout(this.hideMessage, this.notify.timeOut)
    }
  }
}
</script>
