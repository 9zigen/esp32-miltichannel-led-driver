import axios from 'axios'

const host = process.env.NODE_ENV === 'production' ? `http://${document.location.host}/` : 'http://localhost:8081/'

const token = localStorage.getItem('user-token')

if (token) {
  axios.defaults.headers.common['Authorization'] = token
}

export const http = axios.create({
  baseURL: host
})
