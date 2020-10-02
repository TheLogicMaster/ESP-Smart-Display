<template>
  <div>
    <md-card>
      <md-card-header>
        <h1>OTA Update</h1>
      </md-card-header>
      <md-card-content>
        <md-button class="md-accent md-raised" :disabled="progress >= 0" @click="checkForUpdate">Check for Update</md-button>
        <div v-if="this.$store.state.latestVersion !== this.$store.state.stats.version">Display out of date</div>
        <div v-else-if="this.$store.state.latestVersion !== this.$dashboardVersion">Dashboard out of date</div>
        <div v-else>Up to date</div>
        <md-progress-bar v-if="progress >= 0" md-mode="determinate" :md-value="progress"></md-progress-bar>
        <form novalidate class="md-layout" @submit.prevent="submit">
          <md-radio v-model="selected" value="firmware" :disabled="progress >= 0">Firmware</md-radio>
          <md-radio v-model="selected" value="filesystem" :disabled="progress >= 0">Dashboard</md-radio>
          <md-checkbox v-model="manual" :disabled="progress >= 0">Manual</md-checkbox>
          <md-field v-if="manual">
            <label>Binary</label>
            <md-file accept=".bin,.bin.gz" @md-change="onSelect" :disabled="progress >= 0"/>
          </md-field>
          <md-button class="md-accent md-raised" type="submit" :disabled="progress >= 0">Update</md-button>
        </form>
      </md-card-content>
    </md-card>
  </div>
</template>

<script>
import {required} from 'vuelidate/lib/validators'

export default {
  name: "OTAUpdate",
  data: () => ({
    selected: 'firmware',
    progress: -1,
    binary: null,
    inProgress: false,
    manual: false
  }),
  methods: {
    onSelect(files) {
      if (files.length> 0)
        this.binary = files[0]
    },
    async abortUpdate() {
      try {
        this.$axios.get('/abortUpdate').then()
        return true
      } catch (error) {
        console.error(error)
        return false
      }
    },
    async submit() {
      let binary = this.binary

      if (this.manual) {
        if (!this.binary) {
          this.error('Error', 'A binary file is required')
          return
        }
        if (!this.binary.name.endsWith('.bin') && !window.confirm('Are you sure you want to proceed? This does not appear to be an update file.'))
          return
      } else {
        try {
          await this.waitForPromiseSuccess(this.getStats)
          if (this.selected === 'firmware') {
            if (this.$store.state.latestVersion === this.$store.state.stats.version && !window.confirm('It looks like your firmware is up to date, update anyway?'))
              return
          } else {
            if (this.$store.state.latestVersion === this.$dashboardVersion && !window.confirm('It looks like your dashboard is up to date, update anyway?'))
              return
          }
          let assets = (await this.$axios.get(`https://api.github.com/repos/TheLogicMaster/ESP-LED-Matrix-Display/releases/latest`)).data.assets
          let url = ''
          for (let i in assets) {
            if (this.selected === 'firmware' && assets[i].name === `firmware-${this.$store.state.stats.platform}-${this.$store.state.stats.height}x${this.$store.state.stats.width}-v${this.$store.state.latestVersion}.bin` || this.selected !== 'firmware' && assets[i].name === `fs-4m-v${this.$store.state.latestVersion}.bin`)
              url = assets[i].url
          }
          if (url === '') {
            this.error('Error', 'Failed to find binary for board')
            return
          }
          // Hack to bypass CORS for GitHub AWS servers
          url = 'https://cors-anywhere.herokuapp.com/' + url
          binary = (await this.$axios.get(url, {responseType: 'blob', headers: {accept: 'application/octet-stream'}})).data
        } catch (error) {
          this.error('error', 'Failed to update display')
          console.error(error)
        }
      }

      this.$axios.post('/beginUpdate').then(() => {
        let form = new FormData()
        form.append(this.selected, binary)
        this.progress = 0
        this.$axios.post('/update', form, {
          headers: {
            'Content-Type': 'multipart/form-data',
          },
          onUploadProgress: this.onUploadProgress,
        }).then(response => {
          this.progress = -1;
          this.binary = null
          if (response.data.includes('Update Success!')) {
            this.info('Success', 'Successfully updated display', true)
            this.waitForPromiseSuccess(this.getStats).then()
          }
          else {
            this.error('Error', 'Failed to update display: ' + response.data)
            this.waitForPromiseSuccess(this.abortUpdate)
          }
        }).catch(error => {
          this.progress = -1;
          this.error('Error', 'Failed to update display: ' + error)
          this.waitForPromiseSuccess(this.abortUpdate)
        })
      }).catch(error => {
        this.error('Error', 'Failed to update display: ' + error)
      })
    },
    retry() {
      this.submit()
    },
    onUploadProgress(event) {
      this.progress = Math.round((100 * event.loaded) / event.total)
    },
    onCloseAttempt(event) {
      if (this.progress >= 0) {
        event.preventDefault()
        event.returnValue = ""
      }
    }
  },
  beforeRouteLeave(to, from, next) {
    if (this.progress < 0 || window.confirm('Are you really sure you want to leave during an update? This might corrupt the display firmware.'))
      next()
    else
      next(false)
  },
  mounted() {
    window.addEventListener('beforeunload', this.onCloseAttempt, false)
    this.waitForStats().then()
  },
  beforeDestroy() {
    window.removeEventListener('beforeunload', this.onCloseAttempt)
  }
}
</script>

<style scoped>
.md-card {
  width: 320px;
  display: inline-block;
}

.md-progress-bar {
  margin: 24px;
}
</style>
