export function toMinutes (hour, minute) {
  return hour * 60 + minute
}

export function fromMinutes (value) {
  const hour = parseInt(value / 60)
  const minute = value % 60
  return { hour, minute }
}
