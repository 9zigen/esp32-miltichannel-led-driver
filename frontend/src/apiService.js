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
