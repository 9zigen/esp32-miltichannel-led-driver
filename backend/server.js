const express = require('express');
const http = require('http');
const WebSocket = require('ws');
const app = express();
const cors = require('cors');
const bodyParser = require('body-parser');
const fileupload = require("express-fileupload");

//initialize a simple http server
const server = http.createServer(app);

// create application/json parser
var jsonParser = bodyParser.json();

var token;

// Enable CORS on ExpressJS to avoid cross-origin errors when calling this server using AJAX
// We are authorizing all domains to be able to manage information via AJAX (this is just for development)
app.use(cors());
app.options('*', cors()); // include before other routes

// Body parser middleware to auto-parse request text to JSON
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());

app.use(fileupload());

/* auth */
verifyToken = (req, res, next) => {
  let token = req.headers["authorization"];
  if (!token || token === 'undefined') {
    return res.status(401).send({
            message: "Unauthorized!"
          });
    // return res.status(403).send({
    //   message: "No token provided!"
    // });
  }
};

/* Control channels */
app.post('/auth', function (req, res) {
  // res.status(401).send({
  //   message: "Unauthorized!"
  // });
  res.send(JSON.stringify({
    success: true,
    token: 'dsfsdfsdfs'
  }));
});

/* Home page */
app.get('/', function (req, res) {
  res.send('Hello World');
});

/* On-lne Status */
app.get('/api/status', function (req, res) {
  let token = req.headers["authorization"];
  if (!token || token === 'undefined') {
    return res.status(401).send({
      message: "Unauthorized!"
    });
  } else {
    res.send(JSON.stringify({status: status}));
  }
});

/* Control channels */
app.post('/api/light', function (req, res) {
  channels = req.body.channels
  brightness = req.body.brightness
  res.send(JSON.stringify({success: true}));
});

/* OTA */
app.get('/update', function (req, res) {
  res.status(401).send({
    message: "Unauthorized!"
  });
  res.send(JSON.stringify({success: true}));
});

app.post('/upload', function (req, res) {
  console.log(req.files);
  res.send(JSON.stringify({success: true}));
});

/* Schedule ----> */
app.get('/api/schedule', function (req, res) {
  res.send(JSON.stringify({schedule:schedule}));
});

app.post('/api/schedule', function (req, res) {
  if (req.body.schedule) {
    schedule = req.body.schedule
  }
  res.send(JSON.stringify({success: true}));
});

app.post('/api/schedule/status', function (req, res) {
  status.schedule_status = req.body.status
  res.send(JSON.stringify({success: true}));
});

/* Settings ----> */
app.get('/api/settings', function (req, res) {
  res.send(JSON.stringify({
    leds: leds,
    networks: networks,
    services: services,
    thingsboard: thingsboard,
    schedule_config: scheduleConfig,
    time: time,
    cooling: cooling,
    gpio: gpioConfig,
    auth: auth
  }));
});

app.post('/api/settings', function (req, res) {
  console.log(req.body);
  if (req.body.leds) {
    leds = req.body.leds
  }
  if (req.body.networks) {
    networks = req.body.networks
  }
  if  (req.body.services) {
    services = req.body.services
  }
  if  (req.body.thingsboard) {
    thingsboard = req.body.thingsboard
  }
  if (req.body.schedule_config) {
    scheduleConfig = req.body.schedule_config
  }
  if (req.body.time) {
    time = req.body.time
  }
  if (req.body.cooling) {
    cooling = req.body.cooling
  }
  if (req.body.auth) {
    auth = req.body.auth
  }

  res.send(JSON.stringify({success: true}));
});

/* Gpio */
app.get('/api/gpio', function (req, res) {
  res.send(JSON.stringify({
    gpioFunctions: gpioFunctions
  }));
});
app.post('/auth', jsonParser, function(req, res) {
  if (!req.body) return res.sendStatus(400);

  if (req.body.login === "login" && req.body.password === "password") {
    token = Math.random();
    res.json({'success':true, 'token':token});
  }
  else
    res.json({'success':false});

  console.log(req.body);
  // console.log(req.text.pass);
});


const wss = new WebSocket.Server({
  server,
  path: "/ws",
});

let services = {
  hostname: 'test',
  ntp_server: '',
  utc_offset: 0,
  ntp_dst: true,
  mqtt_ip_address: '',
  mqtt_port: '',
  mqtt_user: '',
  mqtt_password: '',
  mqtt_qos: 0,
  enable_ntp: false,
  enable_mqtt: false,
  ota_url: 'http://192.168.4.2:8080/hv_cc_led_driver_rtos.ota.bin'
}

let cooling = {
  installed: true,
  start_temp: 45,
  target_temp: 50,
  max_temp: 70,
  tachometer: 1290,
  pid_kp: 10, /* proportional gain x100 value */
  pid_ki: 10, /* integral gain x100 value */
  pid_kd: 10, /* derivative gain x100 value */
  pid_max: 10, /* maximum value of manipulated variable */
  pid_min: 10 /* minimum value of manipulated variable */
}

let auth = {
  user: 'admin',
  password: '12345678'
}

let thingsboard = {
  token: '',
  endpoint: '',
  qos: 0,
  retain: false,
  enable: false,
  rpc: false
}

let networks = [
  {
    id: 0,
    ssid: 'Best WiFi',
    password: '',
    ip_address: '192.168.1.100',
    mask: '255.255.255.0',
    gateway: '192.168.1.1',
    dns: '192.168.1.1',
    dhcp: false
  },
  {
    id: 1,
    ssid: 'Best WiFi 2',
    password: '',
    ip_address: '',
    mask: '',
    gateway: '',
    dns: '',
    dhcp: true
  }
];

let leds = [
  {
    id: 0,
    color: '#FB647A',
    duty_max: 255,
    power: 50,
    sync_channel: 0,
    sync_channel_group: 0,
    state: 1
  },
  {
    id: 1,
    color: '#6EB96E',
    duty_max: 255,
    power: 30,
    sync_channel: 1,
    sync_channel_group: 0,
    state: 1
  },
  {
    id: 2,
    color: '#42B8F3',
    duty_max: 255,
    power: 40,
    sync_channel: 1,
    sync_channel_group: 1,
    state: 1
  },
  {
    id: 3,
    color: '#b4d9f1',
    duty_max: 255,
    power: 40,
    sync_channel: 0,
    sync_channel_group: 1,
    state: 1
  },
  {
    id: 4,
    color: '#b4d9f1',
    duty_max: 255,
    power: 40,
    sync_channel: 1,
    sync_channel_group: 2,
    state: 1
  },
  {
    id: 5,
    color: '#b4d9f1',
    duty_max: 255,
    power: 40,
    sync_channel: 1,
    sync_channel_group: 1,
    state: 1
  },
  {
    id: 6,
    color: '#b4d9f1',
    duty_max: 255,
    power: 40,
    sync_channel: 1,
    sync_channel_group: 1,
    state: 1
  },
  {
    id: 7,
    color: '#b4d9f1',
    duty_max: 255,
    power: 0,
    sync_channel: 1,
    sync_channel_group: 1,
    state: 1
  }
];

let scheduleConfig = {
  mode: 0, /* 0 -> simple sunrise/sunset; 1 -> multiple points */
  rgb: true,
  duty_range_points: 255, /* 0 - 255 */
  sunrise_hour: 10,
  sunrise_minute: 0,
  sunset_hour: 22,
  sunset_minute: 0,
  simple_mode_duration: 30, /* sunrise/sunset duration in minutes */
  brightness: 100,
  gamma: 100,
  use_sync: 1,
  sync_group: 0,
  duty: [
    10, 20, 10, 20, 20, 4, 10, 0
  ]
}

/* ToDo pin config */
const GPIO_FUNC = {
  NA: 0,
  BRIGTNESS_UP: 1,
  BRIGTNESS_DOWN: 1,
  CHANGE_CHANNEL: 2,
  APPLY: 3
}

/* Board specific external port gpio */
const gpioFunctions = [
  { text: 'Empty', value: GPIO_FUNC.NA },
  { text: 'Increase Brightness', value: GPIO_FUNC.BRIGTNESS_UP },
  { text: 'Decrease Brightness', value: GPIO_FUNC.BRIGTNESS_DOWN },
  { text: 'Change Channel', value: GPIO_FUNC.CHANGE_CHANNEL },
  { text: 'Apply', value: GPIO_FUNC.CHANGE_CHANNEL }
];

let gpioConfig = [
  {
    pin: 25,
    function: GPIO_FUNC.NA,
    alt_function: GPIO_FUNC.NA
  },
  {
    pin: 34,
    function: GPIO_FUNC.NA,
    alt_function: GPIO_FUNC.NA
  },
  {
    pin: 35,
    function: GPIO_FUNC.NA,
    alt_function: GPIO_FUNC.NA
  },
];

let schedule = [
  {
    time_hour: 9,
    time_minute: 0,
    brightness: 50,
    duty: [
      10, 20, 10, 20, 20, 4, 10, 0
    ]
  },
  {
    time_hour: 12,
    time_minute: 0,
    brightness: 255,
    duty: [
      40, 20, 10, 20, 255, 4, 10, 0
    ]
  },
  {
    time_hour: 13,
    time_minute: 0,
    brightness: 120,
    duty: [
      100, 100, 100, 20, 20, 45, 30, 10
    ]
  },
  {
    time_hour: 19,
    time_minute: 0,
    brightness: 20,
    duty: [
      0, 10, 0, 20, 20, 4, 10, 0
    ]
  }

];

let status = {
  upTime: '1 day',
  localTime: '12:22',
  freeHeap: 23567,
  vcc: 48,
  ntc_temperature: 70,
  board_temperature: 25,
  wifiMode: 'STA',
  ipAddress: '192.168.1.199',
  macAddress: '0A:EE:00:00:01:90',
  mqttService: { "enabled": false, "connected": false },
  ntpService: { "enabled": true, "sync": true },
  brightness: 90,
  channels: [50, 255, 100, 20, 20, 0, 10, 5],
  schedule_status: true,
  hardware: "ESP32",
  sdk: "",
  app: "",
  app_version: "",
  app_date: ""
};

let time = {
  year: 20,
  month: 10,
  weekday: 1,
  day: 10,
  hour: 12,
  minute: 1,
  second: 1,
  dst: 0,
  utc: 1
}

let channels = [50, 100, 100, 20, 20, 0, 10, 5];

let brightness = 100;

function getRandomArbitrary(min, max) {
  return Math.random() * (max - min) + min;
}

function getRandomInt(min, max) {
  return Math.floor(Math.random() * (max - min)) + min;
}


function noop() {}

function heartbeat() {
  this.isAlive = true;
}

wss.on('connection', function connection(ws) {
  ws.isAlive = true;
  ws.on('pong', heartbeat);

  ws.on('message', function incoming(message) {
    console.log('received: %s', message);

    let json = JSON.parse(message);
    if (json.command !== undefined) {
      switch (json.command) {
        case ('getStatus'):
          ws.send(JSON.stringify({ status: status }));
          console.log(status);
          break;

        case ('getSettings'):
          ws.send(JSON.stringify({ leds: leds, schedule: schedule }));
          console.log(leds, schedule);
          break;

        case ('saveSettings'):
          // save led color
          leds = json.leds;
          // save schedule
          schedule = json.schedule;
          ws.send(JSON.stringify({ response: 'success' }));
          break;

        case ('getLeds'):
          ws.send(JSON.stringify({ leds: leds }));
          console.log(schedule);
          break;

        case ('setLeds'):
          leds = json.leds;
          ws.send(JSON.stringify({ response: 'success' }));
          break;

        case ('getSchedule'):
          ws.send(JSON.stringify({ schedule: schedule, capacity: 10 }));
          console.log(schedule);
          break;

        case ('setSchedule'):
          schedule = json.schedule;
          ws.send(JSON.stringify({ response: 'success' }));
          break;

        case ('getNetworks'):
          ws.send(JSON.stringify({ networks: networks, capacity: 2 }));
          console.log(schedule);
          break;

        case ('setNetworks'):
          networks = json.networks;
          ws.send(JSON.stringify({ response: 'success' }));
          break;


        default:
          ws.send(Date.now());
          break;
      }

    }

    // setTimeout(function timeout() {
    //   ws.send(Date.now());
    // }, 500);

  });

  // ws.send('something');
});

console.log("Started");

const interval = setInterval(function ping() {
  wss.clients.forEach(function each(ws) {
    if (ws.isAlive === false) return ws.terminate();

    ws.isAlive = false;
    ws.ping(noop);
  });
}, 30000);

function broadcast(data) {
  wss.clients.forEach(function each(client) {
    if (client.readyState === WebSocket.OPEN) {
      client.send(data);
    }
  });
}

//start our server
server.listen(8081, () => {
  console.log(`Server started on port ${server.address().port} :)`);
});
