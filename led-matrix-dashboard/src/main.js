import Vue from 'vue'
import App from './App.vue'
import axios from 'axios'
import Router from 'vue-router'
import Dashboard from "@/components/Dashboard";
import OTAUpdate from "@/components/OTAUpdate";
import VueProgressBar from 'vue-progressbar'
import Vuex from 'vuex';
import VueMoment from 'vue-moment'
import JsonEditor from 'vue-json-edit'

let VueScrollTo = require('vue-scrollto');
Vue.use(VueScrollTo)

Vue.use(JsonEditor)

import VueMobileDetection from 'vue-mobile-detection'

Vue.use(VueMobileDetection)

import Vuelidate from 'vuelidate'

Vue.use(Vuelidate)

Vue.use(VueMoment)
Vue.use(Vuex);

Vue.use(VueProgressBar, {
  color: 'rgb(143, 255, 199)',
  failedColor: 'red',
  height: '2px',
  transition: {
    speed: '0.2s',
    opacity: '0.6s',
    termination: 300
  },
})

//Vue.material.locale.dateFormat = 'dd/MM/yyyy'

import VueMaterial from 'vue-material'
import 'vue-material/dist/vue-material.min.css'
//import 'vue-material/dist/theme/default.css'
import 'vue-material/dist/theme/black-green-light.css'
//import 'vue-material/dist/theme/black-green-light.css'
import Controls from "@/components/Controls";
import Editor from "@/components/Editor";
import Display from "@/components/Display";
import Images from "@/components/Images";
import ImageEditor from "@/components/ImageEditor";
import Settings from "@/components/Settings";
import Jimp from 'jimp'
import download from 'downloadjs'
import interact from 'interactjs'
import Snotify, {SnotifyPosition} from 'vue-snotify'

Vue.use(Snotify, {
  toast: {
    position: SnotifyPosition.rightTop,
    animation: {
      enter: 'bounceInDown',
      exit: 'bounceInDown',
      time: 800
    },
    timeout: 3000
  },
  global: {
    //preventDuplicates: true
  }
})

// Todo: Import only needed VueMaterial components
Vue.use(VueMaterial)

Vue.config.productionTip = false
Vue.use(Router)

Vue.prototype.$download = download
Vue.prototype.$jimp = Jimp
Vue.prototype.$interact = interact
Vue.prototype.$dashboardVersion = Number(process.env.VUE_APP_DASHBOARD_VERSION)
Vue.prototype.$demoMode = Boolean(process.env.VUE_APP_DEMO_MODE)
Vue.prototype.$widgetNames = [
  'Built-in Image',
  'Custom Image',
  'Digital Clock',
  'Analog Clock',
  'Text',
  'JSON GET Text',
  'GET Plain-text',
  'Weather Icon'
]

const UNIQUE_KEY_PROP = '__unique_key_prop__'
const KEY_PREFIX = '__key_prefix__' + Date.now() + '_'
let uid = 0
const isObject = val => val !== null && typeof val === 'object'
const createUniqueKey = obj => {
  if (isObject(obj)) {
    if (UNIQUE_KEY_PROP in obj) {
      return obj[UNIQUE_KEY_PROP]
    }
    const value = KEY_PREFIX + uid++
    Object.defineProperty(obj, UNIQUE_KEY_PROP, {value})
    return value
  }
  return obj
}

Vue.mixin({
  methods: {
    createUniqueKey,
    cloneObject(obj) {
      return JSON.parse(JSON.stringify(obj))
    },
    isObjectEmpty(o) {
      return Object.keys(o).length === 0
    },
    areObjectsEqual(object1, object2) {
      const keys1 = Object.keys(object1);
      const keys2 = Object.keys(object2);
      if (keys1.length !== keys2.length)
        return false;

      for (let key of keys1) {
        const val1 = object1[key];
        const val2 = object2[key];
        const areObjects = isObject(val1) && isObject(val2);
        if (areObjects && !this.areObjectsEqual(val1, val2) || !areObjects && val1 !== val2)
          return false;
      }
      return true;
    },
    sleep(ms) {
      return new Promise(resolve => setTimeout(resolve, ms))
    },
    info(title, content) {
      this.$snotify.info(content, title, {
        closeOnClick: true
      })
    },
    error(title, content) {
      this.$snotify.error(content, title, {
        closeOnClick: true,
        timeout: 0
      })
    },
    confirm(confirm, cancel, title, content) {
      this.$snotify.confirm(content, title, {
        closeOnClick: false,
        position: SnotifyPosition.centerCenter,
        buttons: [
          {
            text: 'Cancel', action: (toast) => {
              this.$snotify.remove(toast.id);
              cancel()
            }, bold: true
          },
          {
            text: 'Yes', action: (toast) => {
              this.$snotify.remove(toast.id);
              confirm()
            }, bold: true
          }
        ]
      }).on("mounted", () => store.commit('set', ['backdrop', true]))
        .on("destroyed", () => store.commit('set', ['backdrop', false]))
        .on("hidden", () => store.commit('set', ['backdrop', false]))
    },
    confirmNavigation(next, title, content) {
      this.confirm(() => next(), () => next(false), title, content)
    },
    confirmUnsavedNavigation(next) {
      this.confirmNavigation(next, 'Unsaved Changes', 'Are you sure you want to leave without saving?')
    },
    async waitForPromiseSuccess(promiseProvider, interval = 500, attempts = 0) {
      let count = 0
      while (attempts === 0 || count < attempts) {
        if (await promiseProvider())
          return true
        count++
        await this.sleep(interval)
      }
      return false
    },
    async waitForConfiguration(interval = 500) {
      while (true) {
        if (!this.isObjectEmpty(store.state.configuration))
          break
        await this.getConfig()
        await this.sleep(interval)
      }
    },
    async waitForStats(interval = 500) {
      while (true) {
        if (!this.isObjectEmpty(store.state.stats))
          break
        await this.getStats()
        await this.sleep(interval)
      }
    },
    async waitForImageData(interval = 500) {
      while (this.isObjectEmpty(store.state.imageData)) {
        await this.getImageData()
        await this.sleep(interval)
      }
    },
    async waitForImage(image, interval = 500) {
      while (!store.state.images.hasOwnProperty(image)) {
        if (store.state.loadingImage === '')
          await this.getImage(image)
        await this.sleep(interval)
      }
    },
    isCppHexValid(hex) {
      return /^0x[0-9A-F]{6}$/i.test(hex)
    },
    isJsHexValid(hex) {
      return /^#[0-9A-F]{6}$/i.test(hex)
    },
    jsHexToCpp(hex) {
      return "0x" + hex.slice(1, hex.length).toUpperCase()
    },
    cppHexToJs(hex) {
      if (hex === null || hex === '')
        return '#000000'
      return "#" + hex.slice(2, hex.length).toLowerCase()
    },
    rgbToHex(r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    },
    hexToRgb(hex) {
      if (hex === undefined) {
        hex = '#000000'
        //console.warn('Tried to convert undefined HEX value')
      }
      let result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
      return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
      } : null;
    },
    unpack565Color(color) {
      return {
        r: Math.round((((color & 0xF800) >> 11) * 255) / 31),
        g: Math.round((((color & 0x07E0) >> 5) * 255) / 63),
        b: Math.round(((color & 0x001F) * 255) / 31)
      }
    },
    pack565Color(color) {
      return ((color.r & 0xF8) << 8) | ((color.g & 0xFC) << 3) | (color.b >> 3)
      //return 0x000000 | (color.r << 24) | (color.g << 16) | (color.b << 8);
    },
    decodeGimpImage(data) {
      let image = []
      for (let i = 0; i + 3 < data.length; i += 4) {
        image.push(this.rgbToHex(
          (((data.charCodeAt(i) - 33) << 2) | ((data.charCodeAt(i + 1) - 33) >> 4)),
          ((((data.charCodeAt(i + 1) - 33) & 0xF) << 4) | ((data.charCodeAt(i + 2) - 33) >> 2)),
          ((((data.charCodeAt(i + 2) - 33) & 0x3) << 6) | ((data.charCodeAt(i + 3) - 33)))))
      }
      return image
    },
    escapeString(string) {
      return (string + '').replace(/[\\"']/g, '\\$&').replace(/\u0000/g, '\\0');
    },
    toBinary(num) {
      return (num >>> 0).toString(2)
    },
    encodeGimpColor(color) {
      let data = ""
      let rgb = this.hexToRgb(color)
      data += String.fromCharCode((rgb.r >> 2) + 33)
      data += String.fromCharCode((((rgb.r & 3) << 4) | ((rgb.g & 0xF0) >> 4)) + 33)
      data += String.fromCharCode((((rgb.g & 0x0F) << 2) | ((rgb.b & 0xC0) >> 6)) + 33)
      data += String.fromCharCode((rgb.b & 0x3F) + 33)
      return this.escapeString(data)
    },
    async getImageData() {
      try {
        const response = await axios.get('/images')
        store.commit('set', ['imageData', response.data])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async saveImageBuffer(name, data, buffer) {
      if (Vue.prototype.$demoMode)
        return true
      let formData = new FormData()
      let blob = new Blob([buffer], {type: 'application/octet-stream'})
      let file = new File([blob], 'image.bin')
      formData.append('image', file)
      try {
        await axios.post('/uploadImage', formData, {
          headers: {'Content-Type': `multipart/form-data`},
          params: {name: name, width: data.width, height: data.height, length: data.length, type: 1}
        })
        return true
      } catch (error) {
        console.log(error)
        return false
      }
    },
    parseImageData(image, data, gimp) {
      let pixels = []
      if (gimp)
        pixels = this.decodeGimpImage(data)
      else {
        let buffer = Buffer.from(data, 'binary')
        let array = new Uint16Array(buffer.buffer, buffer.byteOffset, buffer.length / Uint16Array.BYTES_PER_ELEMENT)
        for (let i = 0; i < array.length; i++) {
          let color = this.unpack565Color(array[i])
          pixels[i] = this.rgbToHex(color.r, color.g, color.b)
        }
      }
      return pixels
    },
    async getImage(image) {
      if (!store.state.imageData[image])
        return false

      store.commit('set', ['loadingImage', image])
      try {
        if (Vue.prototype.$demoMode)
          console.log('Loading demo image')
        const response = await axios.get('/image' + (Vue.prototype.$demoMode ? '/' + image : ''), {
          responseType: store.state.imageData[image].type === 2 ? 'text' : 'arraybuffer',
          params: {image: image + (store.state.imageData[image].progmem ? '_P' : '')}
        })
        store.commit('setImage', [image, this.parseImageData(image, response.data, store.state.imageData[image].type === 2)])
        store.commit('set', ['loadingImage', ''])
        return true
      } catch (error) {
        console.error(error)
        store.commit('set', ['loadingImage', ''])
        return false
      }
    },
    async deleteImage(image) {
      if (Vue.prototype.$demoMode)
        return true
      try {
        await axios.post('/deleteImage', '', {params: {name: image}})
        delete store.state.images[image]
        delete store.state.imageData[image]
        return true
      } catch (error) {
        console.log(error)
        return false
      }
    },
    async refreshImages(full = false) {
      if (full)
        await this.getImageData()
      let success = true
      for (let image in store.state.imageData) {
        if (!full && (image in store.state.images))
          continue
        if (!await this.getImage(image))
          success = false
      }
      return success
    },
    getWidgetMinimumSize(widget) {
      let size = {}
      switch (widget.type) {
        case 0:
        case 1: // Images
          size = {width: widget.width, height: widget.height}
          break
        case 3: // Analog Clock
          size = {width: 10, height: 10}
          break
        case 4: // Text
          if (widget.contentType !== 0)
            size = {width: widget.large ? 6 : 4, height: widget.large ? 7 : 5}
          else
            size = {width: widget.content.length * (widget.large ? 6 : 4), height: widget.large ? 7 : 5}
          break
        case 2: // Digital Clock
          size = {
            width: widget.contentType === 2 ? (widget.large ? 42 : 29) : (widget.large ? 30 : 21),
            height: widget.large ? 7 : 5
          }
          break
        case 5: // GET JSON
        case 6: // GET Plaintext
          size = {width: widget.large ? 6 : 4, height: widget.large ? 7 : 5}
          break
        default:
          let stock = this.createDefaultWidget(widget.type)
          size = {width: stock.width, height: stock.height}
          break
      }
      if (widget.bordered) {
        size.width += 2
        size.height += 2
      }
      return size
    },
    getWidgetDefaults() {
      return {
        id: 0,
        xOff: 0,
        yOff: 0,
        type: 0,
        content: '',
        source: '',
        length: 0,
        offset: 0,
        large: false,
        frequency: 0,
        background: false,
        disabled: false,
        bordered: false,
        borderColor: '0x000000',
        backgroundColor: '0x000000',
        contentType: 0,
        auth: '',
        args: [],
        colors: []
      }
    },
    createDefaultWidget(type) {
      let widget = this.getWidgetDefaults()
      widget.type = type || 0
      switch (widget.type) {
        case 1: // FS Image
          widget.content = 'blm'
        case 0: // Progmem Image
          widget.width = 64
          widget.height = 32
          widget.length = 1
          widget.offset = 0
          widget.background = true
          break
        case 2: // Digital Clock
          widget.width = 21
          widget.height = 5
          widget.frequency = 100
          widget.colors = ['0x00FF00']
          break
        case 3: // Analog Clock
          widget.width = 15
          widget.height = 15
          widget.frequency = 100
          widget.colors = ['0xFF0000', '0x00FF00', '0x0000FF']
          break
        case 4: // Text
          widget.width = 10
          widget.height = 5
          widget.content = 'TEXT'
          widget.colors = ['0x00FF00']
          break
        case 5: // GET JSON
          widget.width = 29
          widget.height = 5
          widget.frequency = 300000
          widget.source = 'http://httpbin.org/get'
          widget.args = ['origin']
          widget.length = 3000
          widget.colors = ['0x00FF00']
          break
        case 7: // Weather Icon
          widget.width = 10
          widget.height = 5
          widget.frequency = 1000
          break
        case 6: // GET Plain-text
          widget.width = 29
          widget.height = 5
          widget.frequency = 300000
          widget.source = ''
          widget.length = 3000
          widget.colors = ['0x00FF00']
          break
      }
      return widget
    },
    async getStats() {
      if (Vue.prototype.$demoMode) {
        store.commit('set', ['stats', {
          uptime: 30000,
          resetReason: 'Software/System restart',
          version: 0,
          fragmentation: 15,
          memoryFree: 17920,
          transparencyBuffer: true,
          filesystemUsed: 614400,
          filesystemTotal: 2070000,
          maxOpenFiles: 5,
          maxPathLength: 31,
          width: 64,
          height: 32,
          vcc: 65535,
          brightness: 100,
          brightnessSensor: 800
        }])
        return true
      }
      try {
        const response = await axios.get('/stats')
        if ((typeof response.data) === 'string') {
          console.error('Invalid JSON response for getStats: ' + response.data)
          return false
        }
        store.commit('set', ['stats', response.data])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    cleanJsonObject(o) {
      for (let attr in o)
        if (o.hasOwnProperty(attr) && (o[attr] === '' || o[attr] === false || o[attr] === '0x000000' || o[attr] === 0 || (Array.isArray(o[attr]) && o[attr].length === 0)))
          delete o[attr]
    },
    cleanConfig(config) {
      this.cleanJsonObject(config)
      config.widgets.forEach(widget => {
        this.cleanJsonObject(widget)
      })
      return config
    },
    bloatWidget(widget) {
      let defaults = this.getWidgetDefaults()
      for (let key of Object.keys(defaults))
        widget[key] = widget[key] || defaults[key]
      return widget
      //return {...this.createDefaultWidget(widget.type), ...widget}
    },
    bloatConfig(config) {
      config.backgroundColor = config.backgroundColor || '0x000000'
      config.metric = config.metric || false
      config.timezone = config.timezone || 'eastern'
      config.weatherLocation = config.weatherLocation || ''
      config.weatherKey = config.weatherKey || ''
      config.brightnessMode = config.brightnessMode || 0
      config.brightnessLower = config.brightnessLower || 15
      config.brightnessUpper = config.brightnessUpper || 100
      config.brightTime = config.brightTime || '09:00'
      config.darkTime = config.darkTime || '20:30'
      config.sensorBright = config.sensorBright || 1023
      config.sensorDark = config.sensorDark || 750
      config.fastUpdate = config.fastUpdate || false
      config.latitude = config.latitude || 42.395060
      config.longitude = config.longitude || -83.369500
      config.widgets = config.widgets || []
      for (let i in config.widgets)
        config.widgets[i] = this.bloatWidget(config.widgets[i])
      return config
    },
    async getConfig() {
      if (Vue.prototype.$demoMode) {
        store.commit('set', ['configuration', this.bloatConfig({
          widgets: [{
            width: 64,
            height: 32,
            content: 'mario',
            background: true,
            length: 1
          },
            {
              id: 1,
              type: 2,
              large: true,
              xOff: 31,
              yOff: 23,
              width: 32,
              height: 9,
              colors: ['0x000000'],
              transparent: true
            }],
          backgroundColor: '0x00FFFF'
        })])
        return true
      }

      try {
        const response = await axios.get('/config')
        if ((typeof response.data) === 'string') {
          console.error('Config file was a string: ' + response.data)
          return false
        }
        store.commit('set', ['configuration', this.bloatConfig(this.cloneObject(response.data))])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async saveConfig(config) {
      if (Vue.prototype.$demoMode)
        return true
      try {
        store.commit('set', ['configuration', this.cloneObject(config)])
        await axios.post('/config', this.cleanConfig(this.cloneObject(config)), {timeout: 10000})
        return true
      } catch (error) {
        console.log(error)
        return false
      }
    }
  }
})

Vue.prototype.$axios = axios

let address = window.location.protocol + '//' + window.location.hostname + window.location.pathname + (window.location.port === '' ? '' : ':' + window.location.port)
if (address.includes(':8080'))
  address = 'http://10.0.0.62:80'
axios.defaults.baseURL = address

Vue.filter('capitalize', function (value) {
  if (!value) return ''
  value = value.toString()
  return value.charAt(0).toUpperCase() + value.slice(1)
})

export const store = new Vuex.Store({
  state: {
    configuration: {},
    imageData: {},
    images: {},
    loadingImage: '',
    stats: {},
    backdrop: false
  },
  mutations: {
    set(state, [variable, value]) {
      state[variable] = value
    },
    setImage(state, [name, image]) {
      state['images'][name] = image
    }
  },
  actions: {},
  getters: {},
});

const routes = [
  {path: '/', redirect: '/dashboard'},
  {path: '/dashboard', component: Dashboard},
  {path: '/display', component: Display},
  {path: '/images', component: Images},
  {path: '/edit_image/:name', component: ImageEditor},
  {path: '/update', component: OTAUpdate},
  {path: '/controls', component: Controls},
  {path: '/editor', component: Editor},
  {path: '/settings', component: Settings}
]

const router = new Router({
  //mode: 'history',
  routes
})

new Vue({
  router,
  store,
  render: h => h(App),
}).$mount('#app')
