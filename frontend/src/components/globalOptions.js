const ledColorOptions = [
  { text: 'Cold White', value: '#B4D9F1' },
  { text: 'Warm White', value: '#FFFDDD' },
  { text: 'Day White', value: '#EAEAEA' },
  { text: 'Actinic', value: '#5bd9ff' },
  { text: 'Super Actinic', value: '#ceb3ff' },
  { text: 'UV', value: '#8A7AD4' },
  { text: 'Deep Blue', value: '#7C9CFF' },
  { text: 'Blue', value: '#42B8F3' },
  { text: 'Cyan', value: '#4DF7FF' },
  { text: 'Emerald', value: '#4DFFC5' },
  { text: 'Green', value: '#6EB96E' },
  { text: 'Yellow', value: '#FDFE90' },
  { text: 'Amber', value: '#FCBB51' },
  { text: 'Red', value: '#FB647A' },
  { text: 'Deep Red', value: '#990000' }
]

const ledGammaOptions = [
  { text: '1.0 (disabled)', value: 100 },
  { text: '1.8', value: 180 },
  { text: '2.0', value: 200 },
  { text: '2.2', value: 220 },
  { text: '2.4', value: 240 }
]

const ledScheduleOptions = [
  { text: 'sunrise/sunset', help: 'two points: sunrise/sunset + moon', value: 0 },
  { text: 'multipoint', help: 'flexible channels control', value: 1 }
]

const tzOptions = [
  { text: 'Pacific/Niue', value: -11 },
  { text: 'US/Hawaii', value: -10 },
  { text: 'America/Anchorage', value: -8 },
  { text: 'America/Los_Angeles', value: -7 },
  { text: 'America/Boise', value: -6 },
  { text: 'America/Chicago', value: -5 },
  { text: 'America/New_York', value: -4 },
  { text: 'America/Aruba', value: -4 },
  { text: 'America/Argentina/Buenos_Aires', value: -3 },
  { text: 'Brazil/DeNoronha', value: -2 },
  { text: 'Atlantic/Azores', value: -1 },
  { text: 'UTC', value: 0 },
  { text: 'Europe/London', value: 1 },
  { text: 'Europe/Madrid', value: 2 },
  { text: 'Europe/Athens', value: 3 },
  { text: 'Europe/Moscow', value: 3 },
  { text: 'Indian/Mahe', value: 4 },
  { text: 'Asia/Ashgabat', value: 5 },
  { text: 'Asia/Dhaka', value: 6 },
  { text: 'Asia/Bangkok', value: 7 },
  { text: 'Asia/Hong_Kong', value: 8 },
  { text: 'Asia/Pyongyang', value: 9 },
  { text: 'Australia/Sydney', value: 10 },
  { text: 'Asia/Magadan', value: 11 }
]

const mqttOptions = [
  { text: 'At most once (0)', value: 0 },
  { text: 'At least once (1)', value: 1 },
  { text: 'Exactly once (2)', value: 2 }
]

export default {
  install (Vue, options) {
    Vue.prototype.$appName = 'HV LED Driver App'
    Vue.prototype.$ledColorOptions = ledColorOptions
    Vue.prototype.$ledGammaOptions = ledGammaOptions
    Vue.prototype.$ledScheduleOptions = ledScheduleOptions
    Vue.prototype.$tzOptions = tzOptions
    Vue.prototype.$mqttOptions = mqttOptions
  }
}
