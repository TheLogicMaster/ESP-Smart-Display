<template>
  <div>
    <md-card>
      <h1>Controls</h1>
      <md-button v-for="(url, name) in commands" class="md-accent md-raised" @click="sendCommand(name)">{{ name }}</md-button>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Controls",
  data: () => ({
    commands: {
      'Send Full Display Refresh': '/fullRefresh',
      'Restart Display': '/restart',
      'Delete all Images': '/deleteAllImages',
      'Reset Display Configuration': '/resetConfiguration',
      'Factory Reset Display': '/factoryReset'
    }
  }),
  methods: {
    sendCommand(name) {
      this.confirm(() => {
        this.$axios.post(this.commands[name]).then(response => {
          if (response.status !== 200)
            this.error('Error', 'Failed to send command to display.')
        }).catch(error => {
          this.error('Error', 'Failed to send command to display: ' + error)
        })
      }, () => {}, 'Send Command', `Are you sure you want to send command '${name}'?`)
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
