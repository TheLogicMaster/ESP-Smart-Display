<template>
  <div>
    <md-card>
      <h1>Settings</h1>
      <div>
        <md-button class="md-layout-item md-accent md-raised" @click="confirmReload">Reload</md-button>
        <md-button class="md-layout-item md-accent md-raised" @click="confirmSave">Save</md-button>
      </div>
      <div class="fields" v-if="config">
        <md-switch v-model="config.metric">Metric Units</md-switch>
        <md-switch v-model="config.fastUpdate">Fast Update Display</md-switch>
        <md-field>
          <label>WeatherMap API Key</label>
          <md-input v-model="config.weatherKey"></md-input>
        </md-field>
        <md-field>
          <label>WeatherMap Location</label>
          <md-input v-model="config.weatherLocation"></md-input>
        </md-field>
        <md-field>
          <label>Timezone</label>
          <md-select v-model="config.timezone">
            <md-option value="eastern">Eastern</md-option>
            <md-option value="central">Central</md-option>
            <md-option value="mountain">Mountain</md-option>
            <md-option value="pacific">Pacific</md-option>
          </md-select>
        </md-field>
        <md-field>
          <label>Scan Pattern</label>
          <md-select v-model.number="config.scanPattern">
            <md-option :value="0">LINE</md-option>
            <md-option :value="1">ZIGZAG</md-option>
            <md-option :value="2">ZZAGG</md-option>
            <md-option :value="3">ZAGGIZ</md-option>
            <md-option :value="4">WZAGZIG</md-option>
            <md-option :value="5">VZAG</md-option>
            <md-option :value="6">ZAGZIG</md-option>
          </md-select>
        </md-field>
        <md-field>
          <label>Mux Pattern</label>
          <md-select v-model.number="config.muxPattern">
            <md-option :value="0">BINARY</md-option>
            <md-option :value="1">STRAIGHT</md-option>
          </md-select>
        </md-field>
        <md-field>
          <label>Mux Delay</label>
          <md-input :min="0" type="number" v-model.number="config.muxDelay"></md-input>
        </md-field>
        <md-field>
          <label>Brightness Mode</label>
          <md-select v-model.number="config.brightnessMode">
            <md-option :value="0">Static</md-option>
            <md-option :value="1">Brightness Sensor</md-option>
            <md-option :value="2">Time of Day</md-option>
            <md-option :value="3">Sunrise</md-option>
          </md-select>
        </md-field>
        <div v-if="config.brightnessMode !== 0" class="input">
          <label class="label">Lower Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="config.brightnessLower"> {{ config.brightnessLower }}%
        </div>
        <div class="input">
          <label v-if="config.brightnessMode === 0" class="label">Brightness</label>
          <label v-else class="label">Upper Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="config.brightnessUpper"> {{ config.brightnessUpper }}%
        </div>
        <div v-if="config.brightnessMode === 1">
          <md-field>
            <label>Max Sensor Brightness</label>
            <md-input :max="1023" v-model.number="config.sensorBright"></md-input>
          </md-field>
          <md-field>
            <label>Min Sensor Brightness</label>
            <md-input :min="0" v-model.number="config.sensorDark"></md-input>
          </md-field>
        </div>
        <div v-if="config.brightnessMode === 2">
          <div class="input">
            <label class="label">Brightening Time</label>
            <input class="time" v-model="config.brightTime" type="time">
          </div>
          <div class="input">
            <label class="label">Dimming Time</label>
            <input class="time" v-model="config.darkTime" type="time">
          </div>
        </div>
        <div v-if="config.brightnessMode === 3">
          <md-field>
            <label>Longitude</label>
            <md-input v-model.number="config.longitude"></md-input>
          </md-field>
          <md-field>
            <label>Latitude</label>
            <md-input v-model.number="config.latitude"></md-input>
          </md-field>
        </div>
      </div>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Settings",
  data: () => ({
    config: {}
  }),
  async mounted() {
    await this.reload()
  },
  methods: {
    confirmReload() {
      if (this.areObjectsEqual(this.$store.state.configuration, this.config))
        this.reload().then()
      else
        this.confirm(() => this.reload().then(), () => {},
            'Reload Settings', 'Are you sure you want to reload the settings? This will erase unsaved changes.')
    },
    async reload() {
      await this.waitForPromiseSuccess(this.getConfig, 500)
      this.config = this.cloneObject(this.$store.state.configuration)
    },
    confirmSave() {
      this.confirm(() => this.save().then(), () => {}, 'Save Settings',
          'Are you sure you want to update the display settings? This will overwrite the current settings.')
    },
    async save() {
      if (await this.saveConfig(this.config))
        this.info('Saved Settings', 'Successfully updated display settings.')
      else
        this.error('Error', 'Failed to update display settings.')
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
