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
import Controls from "@/components/Controls"
import Editor from "@/components/Editor"
import Display from "@/components/Display"
import Images from "@/components/Images"
import ImageEditor from "@/components/ImageEditor"
import Settings from "@/components/Settings"
import Jimp from 'jimp'
import download from 'downloadjs'
import interact from 'interactjs'
import Snotify, {SnotifyPosition} from 'vue-snotify'
import Backup from "@/components/Backup"
import JSZip from "jszip";

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
    areObjectsEqual(object1, object2, excludeKeys = []) {
      const keys1 = Object.keys(object1);
      const keys2 = Object.keys(object2);
      if (keys1.length !== keys2.length)
        return false;

      for (let key of keys1) {
        if (excludeKeys.includes(key))
          continue
        const val1 = object1[key];
        const val2 = object2[key];
        const areObjects = isObject(val1) && isObject(val2);
        if (areObjects && !this.areObjectsEqual(val1, val2) || !areObjects && val1 !== val2)
          return false;
      }
      return true;
    },
    async checkForUpdate() {
      try {
        let result = await axios.get('https://api.github.com/repos/TheLogicMaster/ESP-LED-Matrix-Display/tags')
        let version = 0
        for (let i in result.data)
          version = Math.max(0, Number(result.data[i].name.slice(1)))
        store.commit('set', ['latestVersion', version])
        return version > Vue.prototype.$dashboardVersion
      } catch (error) {
        console.error(error)
        return false
      }
    },
    sleep(ms) {
      return new Promise(resolve => setTimeout(resolve, ms))
    },
    info(title, content, persist=false) {
      this.$snotify.info(content, title, {
        closeOnClick: true,
        timeout: persist ? 0 : 3000
      })
    },
    error(title, content) {
      this.$snotify.error(content, title, {
        closeOnClick: true,
        timeout: 0
      })
    },
    setBackdrop(show) {
      store.commit('set', ['backdrop', show])
    },
    async confirmAsync(title, content) {
      let finished = false
      let confirmed = false
      let finish = canceled => {
        if (finished)
          return
        confirmed = !canceled
        finished = true
        this.setBackdrop(false)
      }
      this.$snotify.confirm(content, title, {
        closeOnClick: false,
        position: SnotifyPosition.centerCenter,
        buttons: [
          {
            text: 'Cancel', action: (toast) => {
              this.$snotify.remove(toast.id);
              finish(true)
            }, bold: true
          },
          {
            text: 'Yes', action: (toast) => {
              this.$snotify.remove(toast.id);
              finish(false)
            }, bold: true
          }
        ]
      }).on("mounted", () => this.setBackdrop(true))
        .on("destroyed", () => finish(true))
        .on("hidden", () => finish(true))
      while (!finished)
        await this.sleep(1)
      return confirmed
    },
    async confirmNavigation(next, title, content) {
      if(await this.confirmAsync(title, content))
        next()
      else
        next(false)
    },
    async confirmUnsavedNavigation(next) {
      await this.confirmNavigation(next, 'Unsaved Changes', 'Are you sure you want to leave without saving?')
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
      if (!hex || hex === '')
        return '#000000'
      return "#" + hex.slice(2, hex.length).toLowerCase()
    },
    rgbToHex(r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    },
    rgbObjectToHex(rgb) {
      return this.rgbToHex(rgb.r, rgb.g, rgb.b)
    },
    parseBinaryFilename(name) {
      let parts = name.split('.')[0].split('-')
      return {
        name: parts[0],
        width: Number(parts[1]),
        height: Number(parts[2]),
        frames: Number(parts[3])
      }
    },
    hexToRgb(hex) {
      if (hex === undefined)
        hex = '#000000'
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
        this.error('Error', 'Failed to upload image to display: ' + error)
        console.log(error)
        return false
      }
    },
    parseImageData(data, gimp = false) {
      let pixels = []
      if (gimp) {
        for (let i = 0; i + 3 < data.length; i += 4)
          pixels.push(this.rgbToHex(
            (((data.charCodeAt(i) - 33) << 2) | ((data.charCodeAt(i + 1) - 33) >> 4)),
            ((((data.charCodeAt(i + 1) - 33) & 0xF) << 4) | ((data.charCodeAt(i + 2) - 33) >> 2)),
            ((((data.charCodeAt(i + 2) - 33) & 0x3) << 6) | ((data.charCodeAt(i + 3) - 33)))))
      }
      else {
        let buffer = Buffer.from(data, 'binary')
        let array = new Uint16Array(buffer.buffer, buffer.byteOffset, buffer.length / Uint16Array.BYTES_PER_ELEMENT)
        for (let i = 0; i < array.length; i++)
          pixels[i] = this.rgbObjectToHex(this.unpack565Color(array[i]))
      }
      return pixels
    },
    async getImage(image) {
      if (!store.state.imageData[image])
        return false

      store.commit('set', ['loadingImage', image])
      try {
        const response = await axios.get('/image' + (Vue.prototype.$demoMode ? '/' + image : ''), {
          responseType: store.state.imageData[image].type === 2 ? 'text' : 'arraybuffer',
          params: {image: image + (store.state.imageData[image].progmem ? '_P' : '')}
        })
        store.commit('setImage', [image, this.parseImageData(response.data, store.state.imageData[image].type === 2)])
        store.commit('set', ['loadingImage', ''])
        return true
      } catch (error) {
        console.error(error)
        store.commit('set', ['loadingImage', ''])
        return false
      }
    },
    validateFilename(name) {
      return /^[0-9a-zA-Z ]+$/.test(name)
    },
    async getRandomName() {
      try {
        return (await this.$axios.get('http://names.drycodes.com/1?combine=2&separator=+&format=json')).data[0]
      } catch (error) {
        console.error(error)
        return 'Random Name'
      }
    },
    async renameImage(image, newName) {
      try {
        await axios.post('/renameImage', '', {params: {name: image, newName: newName}})
        await this.refreshImages(true)
        return true
      } catch (error) {
        this.error('Error', 'Failed to rename image: ' + error)
        console.error(error)
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
    getTextSize(length, font, wrapping) {
      return {
        width: (font === 0 ? 4 : (font === 1 ? 7 : 6 * Math.pow(2, font - 2))) * length + (font === 1 ? 2 : 0),
        height: font === 0 ? 5 : (font === 1 ? 20 : 7 * Math.pow(2, font - 2))
      }
    },
    async downloadBackup() {
      if (await this.refreshImages(true)) {
        let zip = new JSZip()
        let imageDir = zip.folder('images')
        for (let image in store.state.imageData) {
          let data = store.state.imageData[image]
          if (data.progmem)
            continue
          let buffer = new Uint16Array(data.width * data.height * data.length)
          for (let i = 0; i < buffer.length; i++)
            buffer[i] = this.pack565Color(this.hexToRgb(store.state.images[image][i]))
          imageDir.file(`${image}-${data.width}-${data.height}-${data.length}.bin`, buffer.buffer)
        }
        zip.file('config.json', JSON.stringify(store.state.configuration))
        let blob = await zip.generateAsync({type:"blob"})
        download(blob, 'Display-Backup.zip')
      } else {
        this.error('Error', 'Failed to get images for backup')
      }
    },
    async loadBackup(data) {
      try {
        let zip = await JSZip.loadAsync(data)
        await this.saveConfig(JSON.parse(await zip.file('config.json').async('string')))
        let images = {}
        zip.folder('images').forEach((name, file) => images[name] = file)
        for (let image in images) {
          let info = this.parseBinaryFilename(images[image].name.split('images/')[1])
          let attempts = 0
          while (attempts < 3) {
            if (await this.saveImageBuffer(info.name, info, await images[image].async('arraybuffer')))
              break
            attempts++
          }
          if (attempts >= 5) {
            this.error('Error', 'Failed to restore all images, try restoring again')
            return false
          }
        }
        this.info('Success', 'Loaded display backup successfully')
        return true
      } catch (error) {
        console.error(error)
        this.error('Error', 'Failed to load backup: ' + error)
        return false
      }
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
          size = this.getTextSize(widget.content.length, widget.font, widget.contentType)
          break
        case 2: // Digital Clock
          size = this.getTextSize(widget.contentType === 2 ? 7 : 5, widget.font, widget.contentType)
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
        font: 0,
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
    },
    bloatConfig(config) {
      config.backgroundColor = config.backgroundColor || '0x000000'
      config.metric = config.metric || false
      config.timezone = config.timezone || 'America/Detroit'
      config.weatherLocation = config.weatherLocation || ''
      config.weatherKey = config.weatherKey || ''
      config.scanPattern = config.scanPattern || 0
      config.muxPattern = config.muxPattern || 0
      config.muxDelay = config.muxDelay || 0
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
        await axios.post('/config', this.cleanConfig(this.cloneObject(config)), {timeout: 10000, headers: {'Content-Type': 'application/json'}})
        store.commit('set', ['configuration', this.cloneObject(config)])
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
  address = 'http://10.0.0.84:80'
axios.defaults.baseURL = address

Vue.filter('capitalize', function (value) {
  if (!value) return ''
  value = value.toString()
  return value.charAt(0).toUpperCase() + value.slice(1)
})

export const store = new Vuex.Store({
  state: {
    configuration: null,
    imageData: {},
    images: {},
    loadingImage: '',
    stats: null,
    backdrop: false,
    latestVersion: 0
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
  {path: '/settings', component: Settings},
  {path: '/backup', component: Backup}
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
