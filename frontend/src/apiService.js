import { http } from '@/http'

export const getUser = () => new Promise((resolve, reject) => {
  http.get('/api/settings')
    .then(resp => {
      resolve(resp)
    })
    .catch(err => {
      reject(err)
    })
})

export const setUser = user => new Promise((resolve, reject) => {
  http.post('/api/settings', { auth: user })
    .then(resp => {
      resolve(resp)
    })
    .catch(err => {
      reject(err)
    })
})

export const getSettings = () => new Promise((resolve, reject) => {
  http.get('/api/settings')
    .then(resp => {
      resolve(resp)
    })
    .catch(err => {
      reject(err)
    })
})

export const setSettings = settings => new Promise((resolve, reject) => {
  http.post('/api/settings', settings)
    .then(resp => {
      resolve(resp)
    })
    .catch(err => {
      reject(err)
    })
})

export const getGpio = () => new Promise((resolve, reject) => {
  http.get('/api/gpio')
    .then(resp => {
      resolve(resp)
    })
    .catch(err => {
      reject(err)
    })
})
