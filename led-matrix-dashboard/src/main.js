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

// Todo: Import only needed VueMaterial components
Vue.use(VueMaterial)

Vue.config.productionTip = false
Vue.use(Router)

Vue.prototype.$download = download
Vue.prototype.$jimp = Jimp
Vue.prototype.$interact = interact
Vue.prototype.$dashboardVersion = 0
Vue.prototype.$widgetNames = [
  'Built-in Image',
  'Custom Image',
  'Digital Clock',
  'Analog Clock',
  'Text',
  'JSON Get Text',
  'Big Text',
  'Weather Icon',
  'Big Clock',
  'JSON Get Big',
  'Get Plain-text',
  'Big Get Plain-text'
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
    sleep(ms) {
      return new Promise(resolve => setTimeout(resolve, ms))
    },
    async waitForPromiseSuccess(promiseProvider, interval, attempts = 0) {
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
    encodeGimpImage(image) {
      let data = ""
      for (let i = 0; i < image.length; i++) {
        let color = this.hexToRgb(image)
        data.concat(String.fromCharCode((color.r >> 2) + 33))
        data.concat(String.fromCharCode((((color.r & 3) << 4) | ((color.g & 0xF0) >> 4)) + 33))
        data.concat(String.fromCharCode((((color.g & 0x0F) << 2) | ((color.b & 0x60) >> 6)) + 33))
      }
      return data
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
      let formData = new FormData()
      let blob = new Blob([buffer], {type: 'application/octet-stream'})
      let file = new File([blob], 'image.bin')
      formData.append('image', file)
      try {
        await this.$axios.post('/uploadImage', formData, {
          headers: {'Content-Type': `multipart/form-data`},
          params: {name: name, width: data.width, height: data.height, length: data.length, type: data.type}
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
      store.commit('set', ['loadingImage', image])
      try {
        const response = await axios.get('/image', {
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
      try {
        await axios.post('/deleteImage', '', {params: {name: image}})
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
      switch (widget.type) {
        case 0:
        case 1: // Images
          return {width: widget.width, height: widget.height}
        case 3: // Analog Clock
          return {width: 10, height: 10}
        case 4: // Text
          return {width: widget.content.length * 4, height: 5}
        case 6: // Big Text
          return {width: widget.content.length * 6, height: 7}
        case 5:
        case 10: // GET
          return {width: 4, height: 5}
        case 9:
        case 11: // Big GET
          return {width: 6, height: 7}
        default:
          let stock = this.createDefaultWidget(widget.type)
          return {width: stock.width, height: stock.height}
      }
    },
    createDefaultWidget(type) {
      let realType = type || 0
      let widget = {
        id: 0,
        xOff: 0,
        yOff: 0,
        type: realType,
        content: '',
        source: '',
        length: 1,
        offset: 0,
        frequency: 0,
        background: false,
        disabled: false,
        bordered: false,
        borderColor: '0xFF0000',
        backgroundColor: '0x00FFFF',
        contentType: 0,
        auth: '',
        args: [],
        colors: ['0x00FF00']
      }
      switch (realType) {
        case 1: // FS Image
          widget.content = 'blm'
        case 0: // Progmem Image
          widget.width = 64
          widget.height = 32
          widget.length = 1
          widget.offset = 0
          widget.background = true
          widget.colors = []
          break
        case 2: // Digital Clock
          widget.width = 21
          widget.height = 5
          break
        case 3: // Analog Clock
          widget.width = 15
          widget.height = 15
          widget.colors = ['0xFF0000', '0x00FF00', '0x0000FF']
          break
        case 4: // Text
          widget.width = 10
          widget.height = 5
          widget.content = 'TEXT'
          break
        case 5: // GET JSON
          widget.width = 29
          widget.height = 5
          widget.source = 'httpbin.org/get'
          widget.args = ['origin']
          break
        case 6: // Big Text
          widget.width = 29
          widget.height = 7
          widget.content = 'Big Text'
          break
        case 7: // Weather Icon
          widget.width = 10
          widget.height = 5
          widget.colors = []
          break
        case 8: // Big Digital Clock
          widget.width = 29
          widget.height = 7
          break
        case 9: // Big GET JSON
          widget.width = 29
          widget.height = 7
          widget.source = 'httpbin.org/get'
          widget.args = ['origin']
          break
        case 10: // GET Plain-text
          widget.width = 29
          widget.height = 5
          widget.source = ''
          break
        case 11: // Big GET Plain-text
          widget.width = 29
          widget.height = 7
          widget.source = ''
          break
      }
      return widget
    },
    async getStats() {
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
      return {...this.createDefaultWidget(widget.type), ...widget}
    },
    bloatConfig(config) {
      if (!config.backgroundColor)
        config.backgroundColor = '0x000000'
      for (let i in config.widgets)
        config.widgets[i] = this.bloatWidget(config.widgets[i])
      return config
    },
    async getConfig() {
      try {
        const response = await axios.get('/config')
        if ((typeof response.data) === 'string') {
          console.error('Config file was a string: ' + response.data)
          return false
        }
        //console.log(response.data)
        store.commit('set', ['configuration', this.bloatConfig(this.cloneObject(response.data))])
        /*store.commit('set', ['configuration', this.bloatConfig({
          widgets: [{
            id: 0,
            type: 0,
            xOff: 0,
            yOff: 0,
            width: 64,
            height: 32,
            offset: 0,
            content: 'blm',
            disabled: false,
            background: true,
            frequency: 5000,
            length: 2
          },
            {
              id: 1,
              type: 3,
              xOff: 10,
              yOff: 10,
              width: 15,
              height: 15,
              disabled: false,
              background: false,
              frequency: 0,
              length: 1,
              transparent: false
            },
            {
              id: 2,
              type: 4,
              xOff: 0,
              yOff: 0,
              content: 'TEXT',
              width: 15,
              height: 15,
              background: false,
              backgroundColor: '0x00FFFF',
              frequency: 0,
              length: 1,
              transparent: false
            }
          ],
          timezone: 'eastern',
          metric: false,
          backgroundColor: "0x00F51E"
        })])*/
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async saveConfig(config) {
      try {
        store.commit('set', ['configuration', this.cloneObject(config)])
        await axios.post('/config', this.cleanConfig(this.cloneObject(config)))
        return true
      } catch (error) {
        console.log(error)
        return false
      }
    }
  }
})

Vue.prototype.$axios = axios

let address = window.location.protocol + '//' + window.location.hostname + (window.location.port === '' ? '' : ':' + window.location.port)
if (address.includes(':8080'))
  address = 'http://10.0.0.62:80'
axios.defaults.baseURL = address

function prettyResetReason(reason) {
  switch (reason) {
    case 'Hardware Watchdog':
    case 'Exception':
    case 'Software Watchdog':
    case 'Unknown':
      return reason
    default:
      return 'Normal'
  }
}

Vue.filter('capitalize', function (value) {
  if (!value) return ''
  value = value.toString()
  return value.charAt(0).toUpperCase() + value.slice(1)
})

Vue.filter('remove_P', function (value) {
  if (!value) return ''
  value = value.toString()
  return value.endsWith('_P') ? value.slice(0, value.length - 2) : value
})

export const store = new Vuex.Store({
  state: {
    configuration: {},
    imageData: {},
    images: {},
    loadingImage: '',
    stats: {}
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
