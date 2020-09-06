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

// Todo: Import only needed VueMaterial components
Vue.use(VueMaterial)

Vue.config.productionTip = false
Vue.use(Router)

Vue.mixin({
  methods: {
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
    rgbToHex(r, g, b) {
      return "#" + ((1 << 24) + (r << 16) + (g << 8) + b).toString(16).slice(1);
    },
    hexToRgb(hex) {
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
      for(let i = 0; i + 3 < data.length; i += 4) {
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
        let data = response.data;
        store.commit('set', ['imageData', response.data])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async getImage(image) {
      store.commit('set', ['loadingImage', image])
      try {
        let data = []
        if (store.state.imageData[image]["type"] === 2) {
          const response = await axios.get('/image',{params: {image: image}})
          data = this.decodeGimpImage(response.data)
        } else {
          const response = await axios.get('/image',{responseType: 'arraybuffer', params: {image: image}})
          let buffer = Buffer.from(response.data, 'binary')
          let array = new Uint16Array(buffer.buffer, buffer.byteOffset, buffer.length / Uint16Array.BYTES_PER_ELEMENT)
          for (let i = 0; i < store.state.imageData[image]["width"] * store.state.imageData[image]["height"]; i++) {
            let color = this.unpack565Color(array[i])
            data[i] = this.rgbToHex(color.r, color.g, color.b)
          }
        }
        store.commit('setImage', [image, data])
        store.commit('set', ['loadingImage', ''])
        return true
      } catch(error) {
        console.error(error)
        store.commit('set', ['loadingImage', ''])
        return false
      }
    },
    async refreshImages(full) {
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
    async getStats() {
      try {
        const response = await axios.get('/stats')
        store.commit('set', ['stats', {
          'Uptime': Vue.moment.duration(response.data['uptime'], 'seconds').humanize(),
          'Reset Reason': prettyResetReason(response.data['restartReason']),
          'Software Version': response.data['version'],
          'Free Memory': response.data['memoryFree'] + ' bytes',
          'Memory Fragmentation': response.data['fragmentation'] + '%',
          'Display Width': response.data['width'] + ' pixels',
          'Display Height': response.data['height'] + ' pixels'
        }])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async getConfig() {
      try {
        const response = await axios.get('/config')
        store.commit('set', ['configuration', {
          widgets: [{
            id: 0,
            type: 0,
            xOff: 0,
            yOff: 0,
            width: 64,
            height: 32,
            content: 'blm',
            disabled: false,
            background: true,
            frequency: 0,
            length: 1
          }],
          timezone: 'eastern',
          metric: false,
          backgroundColor: "0x00F51E"
        }])
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async saveConfig() {
      try {
        await axios.post('/config', store.state.configuration)
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
    stats: {
      'Uptime': '',
      'Reset Reason': '',
      'Software Version': '',
      'Free Memory': '',
      'Memory Fragmentation': '',
      'Display Width': '',
      'Display Height': ''
    }
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
