import Vue from 'vue'

export const store = Vue.observable({
  isAuthenticated: !!localStorage.getItem('user-token')
})

export const mutations = {
  setAuthenticated (token) {
    /* store the token in localstorage */
    localStorage.setItem('user-token', token)
    store.isAuthenticated = true
  },
  logout () {
    store.isAuthenticated = false
    localStorage.removeItem('user-token')
  }
}
