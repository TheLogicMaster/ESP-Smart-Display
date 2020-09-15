<template>
  <div>
    <md-card>
      <h1>Controls</h1>
      <md-button class="md-accent md-raised" @click="sendCommand('/fullRefresh')">Send Full Display Refresh</md-button>
      <md-button class="md-accent md-raised" @click="sendCommand('/restart')">Restart Display</md-button>
      <md-button class="md-accent md-raised" @click="sendCommand('/resetConfiguration')">Reset Display Configuration</md-button>
      <md-button class="md-accent md-raised" @click="sendCommand('/factoryReset')">Factory Reset Display</md-button>
    </md-card>

    <md-dialog-alert
        :md-active.sync="errorAlert"
        md-content="Failed to send command to display"
        md-confirm-text="Close"/>
  </div>
</template>

<script>
export default {
  name: "Controls",
  data: () => ({
    errorAlert: false
  }),
  methods: {
    sendCommand(url) {
      this.$axios.post(url).then(response => {
        if (response.status !== 200)
          this.errorAlert = true;
      }).catch(error => {
        console.error(error)
        this.errorAlert = true;
      })
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
