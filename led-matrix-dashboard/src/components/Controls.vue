<template>
  <div>
    <md-card>
      <h1>Controls</h1>
      <div v-for="(command, name) in commands">
        <md-button class="md-accent md-raised" @click="sendCommand(name)">{{ name }}</md-button>
        <md-tooltip md-delay="500" md-direction="left"> {{ command.tooltip }} </md-tooltip>
      </div>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Controls",
  data: () => ({
    commands: {
      'Full Display Refresh': {url: '/fullRefresh', tooltip: 'Fully re-draw the display'},
      'Force Update Widgets': {url: '/forceUpdate', tooltip: 'Force all widgets to update'},
      'Refresh Weather': {url: '/refreshWeather', tooltip: 'Refresh current weather data'},
      'Refresh Sunrise/Sunset': {url: '/refreshSunMoon', tooltip: 'Refresh the sunset/sunrise times'},
      'Refresh Timezone': {url: '/refreshTimezone', tooltip: 'Refresh the timezone offset'},
      'Refresh Time': {url: '/refreshTime', tooltip: 'Refresh the current time'},
      'Restart Display': {url: '/restart', tooltip: 'Restart the display'},
      'Delete all Images': {url: '/deleteAllImages', tooltip: 'Delete all custom images'},
      'Reset Display Configuration': {url: '/resetConfiguration', tooltip: 'Reset display configuration file'},
      'Factory Reset Display': {url: '/factoryReset', tooltip: 'Reset display configuration and erase custom images'}
    }
  }),
  methods: {
    async sendCommand(name) {
      if (!(await this.confirm('Send Command', `Are you sure you want to send command '${name}'?`)))
        return

      try {
        let response = await this.$axios.post(this.commands[name].url)
        if (response.status !== 200)
          this.error('Error', 'Failed to send command to display.')
      } catch (error) {
        this.error('Error', 'Failed to send command to display: ' + error)
        console.error(error)
      }
    }
  }
}
</script>

<style scoped>
.md-card {
  max-width: 300px;
  display: inline-block;
}
</style>
