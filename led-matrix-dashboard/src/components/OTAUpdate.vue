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
          <md-radio v-model="selected" value="filesystem" :disabled="progress >= 0">FileSystem</md-radio>
          <md-checkbox v-model="manual" :disabled="progress >= 0">Manual</md-checkbox>
          <md-field v-if="manual">
            <label>Binary</label>
            <md-file v-model="file" accept=".bin,.bin.gz" @md-change="onSelect" :disabled="progress >= 0"/>
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
    file: '',
    manual: false
  }),
  methods: {
    onSelect(file) {
      this.binary = file[0]
    },
    async submit() {
      if (this.manual) {
        if (!this.binary) {
          this.error('Error', 'A binary file is required')
          return
        }

        if (!this.file.endsWith('.bin') && !window.confirm('Are you sure you want to proceed? This does not appear to be an update file.'))
          return

        this.$axios.post('/beginUpdate').then(() => {
          let form = new FormData()
          form.append(this.selected, this.binary)
          this.progress = 0
          this.$axios.post('/update', form, {
            headers: {
              'Content-Type': 'multipart/form-data',
            },
            onUploadProgress: this.onUploadProgress,
          }).then(response => {
            this.progress = -1;
            this.file = ''
            this.info('Success', 'Successfully updated display')
          }).catch(error => {
            this.progress = -1;
            this.error('Error', 'Failed to update display: ' + error)
          })
        }).catch(error => {
          this.error('Error', 'Failed to update display: ' + error)
        })
      } else {
        let result = this.$axios.get('')
      }
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
