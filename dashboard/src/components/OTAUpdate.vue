<template>
  <div>
    <md-card>
      <md-card-header>
        <h1>OTA Update</h1>
      </md-card-header>
      <md-card-content>
        <div class="inline-centered">
          <md-button class="md-accent md-raised" :disabled="progress >= 0" @click="checkForUpdate">Check for Update
          </md-button>
          <md-tooltip md-delay="1000" md-direction="bottom"> Check GitHub for updates</md-tooltip>
        </div>
        <div v-if="this.$store.state.latestVersion !== this.$store.state.stats.version">Display out of date</div>
        <div v-else-if="this.$store.state.latestVersion !== this.$dashboardVersion">Dashboard out of date</div>
        <div v-else>Up to date</div>
        <md-progress-bar v-if="progress >= 0" md-mode="determinate" :md-value="progress"></md-progress-bar>
        <form novalidate class="md-layout" @submit.prevent="updateDialog = true">
          <div class="inline-centered">
            <md-radio v-model="selected" value="firmware" :disabled="progress >= 0">Firmware</md-radio>
            <md-tooltip md-delay="1000" md-direction="bottom"> Firmware update mode</md-tooltip>
          </div>
          <div class="inline-centered">
            <md-radio v-model="selected" value="filesystem" :disabled="progress >= 0">Dashboard</md-radio>
            <md-tooltip md-delay="1000" md-direction="bottom"> Dashboard update mode</md-tooltip>
          </div>
          <div class="inline-centered">
            <md-checkbox v-model="manual" :disabled="progress >= 0">Manual</md-checkbox>
            <md-tooltip md-delay="1000" md-direction="bottom"> Manually flash using a binary file</md-tooltip>
          </div>
          <md-field v-if="manual">
            <label>Binary</label>
            <md-file accept=".bin,.bin.gz" @md-change="onSelect" :disabled="progress >= 0"/>
            <md-tooltip md-delay="1000" md-direction="bottom"> The binary file to flash</md-tooltip>
          </md-field>
          <div class="inline-centered">
            <md-button class="md-accent md-raised" type="submit" :disabled="progress >= 0">Update</md-button>
            <md-tooltip md-delay="1000" md-direction="bottom"> Perform software update</md-tooltip>
          </div>
        </form>
      </md-card-content>
    </md-card>

    <md-dialog :md-active.sync="updateDialog">
      <md-dialog-title>Update Display</md-dialog-title>
      <md-dialog-content style="width: 300px">
        Are you sure you want to update the display? The display will be factory reset in the process, so make sure you
        have
        saved a backup of the configuration before updating. All custom images and settings will be lost.
      </md-dialog-content>
      <md-dialog-actions>
        <md-button class="md-accent md-raised" @click="updateDialog = false">Cancel</md-button>
        <md-button class="md-accent md-raised" @click="downloadBackup">Download Backup</md-button>
        <md-button class="md-accent md-raised" @click="update">Update</md-button>
      </md-dialog-actions>
    </md-dialog>
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
    manual: false,
    updateDialog: false
  }),
  methods: {
    onSelect(files) {
      // Todo: switch to normal file input if md-file blank file glitch isn't fixed
      if (files.length > 0)
        this.binary = files[0]
    },
    async update() {
      this.updateDialog = false
      let binary = this.binary

      if (this.manual) {
        if (!this.binary) {
          this.error('Error', 'A binary file is required')
          return
        }
        if (!this.binary.name.endsWith('.bin') && !(await this.confirm('Update Binary', 'Are you sure you want to proceed? This does not appear to be an update file')))
          return
      } else {
        try {
          await this.waitForPromiseSuccess(this.getStats)
          if (this.selected === 'firmware') {
            if (this.$store.state.latestVersion === this.$store.state.stats.version && !(await this.confirm('Update Firmware', 'It looks like your firmware is up to date, update anyway?')))
              return
          } else {
            if (this.$store.state.latestVersion === this.$dashboardVersion && !(await this.confirm('Update Dashboard', 'It looks like your dashboard is up to date, update anyway?')))
              return
          }
          let assets = (await this.$axios.get(`https://api.github.com/repos/TheLogicMaster/ESP-Smart-Display/releases/latest`)).data.assets
          let url = ''
          for (let i in assets) {
            if (this.selected === 'firmware' && assets[i].name === `firmware-${this.$store.state.stats.platform}-v${this.$store.state.latestVersion}.bin` || this.selected !== 'firmware' && assets[i].name === `fs-${this.$store.state.stats.features & 1 ? 'esp32' : 'esp8266'}-v${this.$store.state.latestVersion}.bin`)
              url = assets[i].url
          }
          if (url === '') {
            this.error('Error', 'Failed to find binary for board')
            return
          }
          // Hack to bypass CORS for GitHub AWS servers
          url = 'https://cors-anywhere.herokuapp.com/' + url
          binary = (await this.$axios.get(url, {
            responseType: 'blob',
            headers: {accept: 'application/octet-stream'}
          })).data
        } catch (error) {
          this.error('error', 'Failed to update display')
          console.error(error)
        }
      }

      try {
        let form = new FormData()
        form.append(this.selected, binary, this.selected)
        this.progress = 0
        let response = await this.$axios.post('/update', form, {
          headers: {
            'Content-Type': 'multipart/form-data',
          },
          onUploadProgress: this.onUploadProgress,
        })
        this.progress = -1;
        this.binary = null
        if (response.data.includes('OK')) {
          this.info('Success', 'Successfully updated display', true)
          this.waitForPromiseSuccess(this.getStats).then()
        } else
          this.error('Error', 'Failed to update display: ' + response.data)
      } catch (error) {
        this.error('Error', 'Failed to update display: ' + error)
        this.progress = -1;
        console.error(error)
      }
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
  async beforeRouteLeave(to, from, next) {
    if (this.progress < 0 || await this.confirm('Danger', 'Are you really sure you want to leave during an update? This might corrupt the display firmware.'))
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
