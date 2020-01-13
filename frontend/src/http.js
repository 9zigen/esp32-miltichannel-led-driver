import axios from 'axios'

const host = process.env.NODE_ENV === 'production' ? `http://${document.location.host}/` : 'http://localhost:8081/'

export const http = axios.create({
  baseURL: host,
  headers: {
    Authorization: 'Bearer {token}'
  }
})
