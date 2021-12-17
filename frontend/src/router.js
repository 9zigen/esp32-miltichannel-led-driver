import Vue from 'vue'
import Router from 'vue-router'
import Home from './views/Home'
import Settings from './views/Settings'
import About from './views/About'
import Schedule from './views/Schedule'
import Wifi from './views/Wifi'
import Login from './views/Login'
import { store } from '@/store'

const ifNotAuthenticated = (to, from, next) => {
  if (store.isAuthenticated) {
    next()
    return
  }
  next('/login')
}

Vue.use(Router)

export default new Router({
  base: process.env.BASE_URL,
  linkActiveClass: 'is-active',
  routes: [
    {
      path: '/',
      name: 'home',
      component: Home,
      beforeEnter: ifNotAuthenticated
    },
    {
      path: '/schedule',
      name: 'schedule',
      component: Schedule,
      beforeEnter: ifNotAuthenticated
    },
    {
      path: '/wifi',
      name: 'wifi',
      component: Wifi,
      beforeEnter: ifNotAuthenticated
    },
    {
      path: '/settings',
      name: 'settings',
      component: Settings,
      beforeEnter: ifNotAuthenticated
    },
    {
      path: '/about',
      name: 'about',
      component: About
    },
    {
      path: '/login',
      name: 'login',
      // props: { wasRebooted: false },
      component: Login
    }
  ]
})
