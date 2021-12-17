import { http } from '@/http'
import { mutations } from '@/store'

export const authService = user => new Promise((resolve, reject) => {
  http({ url: '/auth', data: user, method: 'POST' })
    .then(resp => {
      const token = resp.data.token
      http.defaults.headers.common['Authorization'] = token
      mutations.setAuthenticated(token)
      resolve(resp)
    })
    .catch(err => {
      /* if the request fails, remove any possible user token if possible */
      localStorage.removeItem('user-token')
      reject(err)
    })
})
