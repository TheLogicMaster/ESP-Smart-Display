<template>
  <div>
    <md-card>
      <md-card-header>
        <h1>OTA Update</h1>
      </md-card-header>
      <md-card-content>
        <md-progress-bar v-if="progress >= 0" md-mode="determinate" :md-value="progress"></md-progress-bar>
        <form novalidate class="md-layout" @submit.prevent="submit">
          <md-radio v-model="selected" value="firmware" :disabled="inProgress">Firmware</md-radio>
          <md-radio v-model="selected" value="filesystem" :disabled="inProgress">FileSystem</md-radio>
          <md-field>
            <label>Binary</label>
            <md-file v-model="file" accept=".bin,.bin.gz" @md-change="onSelect" :disabled="inProgress"/>
          </md-field>
          <md-button class="md-accent md-raised" type="submit" :disabled="inProgress">Update</md-button>
        </form>

        <md-snackbar :md-active.sync="errorSnackbar" md-persistent class="md-layout">
          <span class="md-layout-item md-size-60"> {{ error }} </span>
          <div class="md-layout-item md-size-40">
            <md-button class="md-accent md-raised" @click="retry"> Retry</md-button>
            <md-button class="md-accent md-raised" @click="errorSnackbar = false"> Close</md-button>
          </div>

        </md-snackbar>

        <md-snackbar :md-active.sync="successSnackbar" md-persistent>
          <span> Successfully updated display </span>
          <md-button @click="successSnackbar = false"> Okay</md-button>
        </md-snackbar>
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
    errorSnackbar: false,
    error: '',
    successSnackbar: false,
    file: ''
  }),
  methods: {
    onSelect(file) {
      this.binary = file[0]
    },
    submit() {
      /*this.$v.$touch()
      if (this.$v.$invalid) {
        return
      }*/
      if (!this.binary) {
        this.error = 'A binary file is required'
        this.errorSnackbar = true;
        return
      }

      if (!this.file.endsWith('.bin') && !window.confirm('Are you sure you want to proceed? This does not appear to be an update file.'))
        return

      this.$axios.post('/beginUpdate').then(() => {
        let form = new FormData()
        form.append(this.selected, this.binary)
        this.progress = 0
        this.inProgress = true;
        this.$axios.post('/update', form, {
          headers: {
            'Content-Type': 'multipart/form-data',
          },
          onUploadProgress: this.onUploadProgress,
        }).then(response => {
          this.inProgress = false;
          this.progress = -1;
          this.file = ''
          this.successSnackbar = true
          //alert(response.data)
          //this.$router.push('/dashboard')
        }).catch(error => {
          this.inProgress = false;
          this.progress = -1;
          this.error = 'Failed to update display: ' + error
          this.errorSnackbar = true;
        })
      }).catch(error => {
        this.error = 'Failed to update display: ' + error
        this.errorSnackbar = true;
      })
    },
    retry() {
      this.errorSnackbar = false
      this.submit()
    },
    onUploadProgress(event) {
      this.progress = Math.round((100 * event.loaded) / event.total)
    },
    onCloseAttempt(event) {
      if (this.inProgress) {
        event.preventDefault()
        event.returnValue = ""
      }
    }
  },
  beforeRouteLeave(to, from, next) {
    if (this.inProgress) {
      if (window.confirm('Are you really sure you want to leave during an update? This might corrupt the display firmware.'))
        next()
      else
        next(false)
    } else {
      next()
    }
  },
  mounted() {
    window.addEventListener('beforeunload', this.onCloseAttempt, false)
  },
  beforeDestroy() {
    window.removeEventListener('beforeunload', this.onCloseAttempt)
  }
  /*validations: {
    binary: {
      required
    }
  }*/
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
