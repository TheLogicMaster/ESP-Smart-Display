<template>
  <div>
    <md-card>
      <h1>Settings</h1>
      <div>
        <div class="inline-centered">
          <md-button class="md-accent md-raised" @click="confirmReload">Reload</md-button>
          <md-tooltip md-delay="1000" md-direction="bottom"> Reload display settings </md-tooltip>
        </div>
        <div class="inline-centered">
          <md-button class="md-accent md-raised" @click="confirmSave">Save</md-button>
          <md-tooltip md-delay="1000" md-direction="bottom"> Save display settings </md-tooltip>
        </div>
      </div>
      <div class="fields" v-if="config">
        <div>
          <md-switch v-model="config.metric">Metric Units</md-switch>
          <md-tooltip md-delay="1000" md-direction="bottom"> Use metric units </md-tooltip>
        </div>
        <div>
          <md-switch v-model="config.vertical">Vertical Orientation</md-switch>
          <md-tooltip md-delay="1000" md-direction="bottom"> Render display vertically </md-tooltip>
        </div>
        <div>
          <md-switch v-model="config.fastUpdate">Fast Update Display</md-switch>
          <md-tooltip md-delay="1000" md-direction="bottom"> PxMatrix Fast Update mode </md-tooltip>
        </div>
        <md-field>
          <label>WeatherMap API Key</label>
          <md-input v-model="config.weatherKey"></md-input>
          <md-tooltip md-delay="1000" md-direction="bottom"> OpenWeatherMap API key </md-tooltip>
        </md-field>
        <md-field>
          <label>WeatherMap Location</label>
          <md-input v-model="config.weatherLocation"></md-input>
          <md-tooltip md-delay="1000" md-direction="bottom"> OpenWeatherMap location id </md-tooltip>
        </md-field>
        <div>
          <md-switch v-model="automaticTimezone" @change="changeAutoTimezone">Automatic IP Based Timezone</md-switch>
          <md-tooltip md-delay="1000" md-direction="bottom"> Get timezone from IP country </md-tooltip>
        </div>
        <md-autocomplete v-if="!automaticTimezone" v-model="config.timezone" :md-options="timezones">
          <label>Timezone</label>
          <md-tooltip md-delay="1000" md-direction="bottom"> Timezone location </md-tooltip>
        </md-autocomplete>
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
          <md-tooltip md-delay="1000" md-direction="bottom"> The PxMatrix scan pattern </md-tooltip>
        </md-field>
        <md-field>
          <label>Mux Pattern</label>
          <md-select v-model.number="config.muxPattern">
            <md-option :value="0">BINARY</md-option>
            <md-option :value="1">STRAIGHT</md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="bottom"> The PxMatrix mux pattern </md-tooltip>
        </md-field>
        <md-field>
          <label>Mux Delay</label>
          <md-input :min="0" type="number" v-model.number="config.muxDelay"></md-input>
          <md-tooltip md-delay="1000" md-direction="bottom"> The PxMatrix mux delay </md-tooltip>
        </md-field>
        <md-field>
          <label>Brightness Mode</label>
          <md-select v-model.number="config.brightnessMode">
            <md-option :value="0">Static</md-option>
            <md-option :value="1">Brightness Sensor</md-option>
            <md-option :value="2">Time of Day</md-option>
            <md-option :value="3">Sunrise</md-option>
          </md-select>
          <md-tooltip md-delay="1000" md-direction="bottom"> The display brightness control mode </md-tooltip>
        </md-field>
        <div v-if="config.brightnessMode !== 0" class="input">
          <label class="label">Lower Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="config.brightnessLower"> {{ config.brightnessLower }}%
          <md-tooltip md-delay="1000" md-direction="bottom"> The lower brightness percentage </md-tooltip>
        </div>
        <div class="input">
          <label v-if="config.brightnessMode === 0" class="label">Brightness</label>
          <label v-else class="label">Upper Brightness</label>
          <input min="1" type="range" class="slider" v-model.number="config.brightnessUpper"> {{ config.brightnessUpper }}%
          <md-tooltip md-delay="1000" md-direction="bottom"> The higher brightness percentage </md-tooltip>
        </div>
        <div v-if="config.brightnessMode === 1">
          <md-field>
            <label>Max Sensor Brightness</label>
            <md-input :max="1023" v-model.number="config.sensorBright"></md-input>
            <md-tooltip md-delay="1000" md-direction="bottom"> The higher brightness sensor value </md-tooltip>
          </md-field>
          <md-field>
            <label>Min Sensor Brightness</label>
            <md-input :min="0" v-model.number="config.sensorDark"></md-input>
            <md-tooltip md-delay="1000" md-direction="bottom"> The lower brightness sensor value </md-tooltip>
          </md-field>
        </div>
        <div v-if="config.brightnessMode === 2">
          <div class="input">
            <label class="label">Brightening Time</label>
            <input class="time" v-model="config.brightTime" type="time">
            <md-tooltip md-delay="1000" md-direction="bottom"> The time of day to brighten display </md-tooltip>
          </div>
          <div class="input">
            <label class="label">Dimming Time</label>
            <input class="time" v-model="config.darkTime" type="time">
            <md-tooltip md-delay="1000" md-direction="bottom"> The time of day to dim display </md-tooltip>
          </div>
        </div>
        <div v-if="config.brightnessMode === 3">
          <md-field>
            <label>Longitude</label>
            <md-input v-model.number="config.longitude"></md-input>
            <md-tooltip md-delay="1000" md-direction="bottom"> Geographic longitude coordinate </md-tooltip>
          </md-field>
          <md-field>
            <label>Latitude</label>
            <md-input v-model.number="config.latitude"></md-input>
            <md-tooltip md-delay="1000" md-direction="bottom"> Geographic latitude coordinate </md-tooltip>
          </md-field>
        </div>
      </div>
    </md-card>
  </div>
</template>

<script>
import moment from 'moment-timezone'
export default {
  name: "Settings",
  data: () => ({
    config: {},
    timezones: [],
    automaticTimezone: false
  }),
  async mounted() {
    this.timezones = moment.tz.names()
    await this.reload()
  },
  methods: {
    changeAutoTimezone(automatic) {
      if (automatic)
        this.config.timezone = 'GeoIP'
      else
        this.config.timezone = 'America/Detroit'
    },
    async confirmReload() {
      if (this.areObjectsEqual(this.$store.state.configuration, this.config) || (await this.confirm('Reload Settings', 'Are you sure you want to reload the settings? This will erase unsaved changes.')))
        await this.reload()
    },
    async reload() {
      await this.waitForPromiseSuccess(this.getConfig, 500)
      this.config = this.cloneObject(this.$store.state.configuration)
      this.automaticTimezone = this.config.timezone.length === 0
    },
    async confirmSave() {
      if (await this.confirm('Save Settings','Are you sure you want to update the display settings? This will overwrite the current settings.'))
        await this.save()
    },
    async save() {
      if (await this.saveConfig(this.config)) {
        this.info('Saved Settings', 'Successfully updated display settings.')
        await this.getStats()
      }
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
