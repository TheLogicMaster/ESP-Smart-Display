<template>
  <div>
    <md-card>
      <h1>Settings</h1>
      <div>
        <md-button class="md-layout-item md-accent md-raised" @click="reloadConfirm = true">Reload</md-button>
        <md-button class="md-layout-item md-accent md-raised" @click="saveConfirm = true">Save</md-button>
      </div>
      <div class="fields">
        <md-switch v-model="metric">Metric Units</md-switch>
        <md-switch v-model="fastUpdate">Fast Update Display</md-switch>
        <md-field>
          <label>WeatherMap API Key</label>
          <md-input v-model="weatherKey"></md-input>
        </md-field>
        <md-field>
          <label>WeatherMap Location</label>
          <md-input v-model="weatherLocation"></md-input>
        </md-field>
        <md-field>
          <label>Timezone</label>
          <md-select v-model="timezone">
            <md-option value="eastern">Eastern</md-option>
            <md-option value="central">Central</md-option>
            <md-option value="mountain">Mountain</md-option>
            <md-option value="pacific">Pacific</md-option>
          </md-select>
        </md-field>
        <md-field>
          <label>Brightness Mode</label>
          <md-select v-model.number="brightnessMode">
            <md-option :value="0">Fixed</md-option>
            <md-option :value="1">Automatic</md-option>
            <md-option :value="2">Time of Day</md-option>
            <md-option :value="3">Sunrise</md-option>
          </md-select>
        </md-field>
        <div v-if="brightnessMode !== 0" class="input">
          <label class="label">Lower Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="brightnessLower"> {{ brightnessLower }}%
        </div>
        <div class="input">
          <label v-if="brightnessMode === 0" class="label">Brightness</label>
          <label v-else class="label">Upper Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="brightnessUpper"> {{ brightnessUpper }}%
        </div>
        <div v-if="brightnessMode === 2">
          <div class="input">
            <label class="label">Brightening Time</label>
            <input class="time" v-model="brightTime" type="time">
          </div>
          <div class="input">
            <label class="label">Dimming Time</label>
            <input class="time" v-model="darkTime" type="time">
          </div>
        </div>
      </div>
    </md-card>

    <md-dialog-confirm
        :md-active.sync="reloadConfirm"
        md-title="Reload Settings"
        md-content="Are you sure you want to reload the settings? This will erase unsaved changes."
        md-confirm-text="Reload"
        md-cancel-text="Cancel"
        @md-confirm="reload" />
    <md-dialog-confirm
        :md-active.sync="saveConfirm"
        md-title="Save Settings"
        md-content="Are you sure you want to save the settings?"
        md-confirm-text="Save"
        md-cancel-text="Cancel"
        @md-confirm="save" />
  </div>
</template>

<script>
export default {
  name: "Settings",
  data: () => ({
    metric: false,
    fastUpdate: false,
    timezone: 'eastern',
    weatherLocation: '',
    weatherKey: '',
    brightnessMode: 0,
    brightnessUpper: 100,
    brightnessLower: 1,
    brightTime: '09:00',
    darkTime: '20:30',
    reloadConfirm: false,
    saveConfirm: false
  }),
  async mounted() {
    await this.reload()
  },
  methods: {
    async reload() {
      // Todo: Switch to Display.vue method of configuration copy
      await this.waitForPromiseSuccess(this.getConfig, 500)
      let config = this.$store.state.configuration
      this.metric = config.metric || false
      this.timezone = config.timezone || 'eastern'
      this.weatherLocation = config.weatherLocation || ''
      this.weatherKey = config.weatherKey || ''
      this.brightnessMode = config.brightnessMode || 0
      this.brightnessLower = config.brightnessLower || 1
      this.brightnessUpper = config.brightnessUpper || 100
      this.brightTime = config.brightTime || '09:00'
      this.darkTime = config.darkTime || '20:30'
      this.fastUpdate = config.fastUpdate || false
    },
    async save() {
      let config = this.cloneObject(this.$store.state.configuration)
      config.metric = this.metric
      config.timezone = this.timezone
      config.weatherLocation = this.weatherLocation
      config.weatherKey = this.weatherKey
      config.brightnessMode = this.brightnessMode
      config.brightnessLower = this.brightnessLower
      config.brightnessUpper = this.brightnessUpper
      config.brightTime = this.brightTime
      config.darkTime = this.darkTime
      config.fastUpdate = this.fastUpdate
      if (await this.saveConfig(config))
        ;
      else
        ;
    }
  }
}
</script>

<style scoped>
.md-card {
  width: 400px;
  display: inline-block;
}

.input {
  display: flex;
  margin-top: 15px;
  margin-bottom: 15px;
}

.slider {
  margin-right: 10px;
}

.label {
  margin-right: 10px;
  width: 120px;
  display: flex;
}

.time {
  width: 130px;
}

.fields {
  margin-left: 10px;
  //display: inline-block;
}

.md-switch {
  display: flex;
}

.md-field {
  width: 350px;
}
</style>
