<template>
  <div>
    <md-card>
      <md-card-header>
        <h1>
          Backup and Restore
        </h1>
      </md-card-header>
      <h3>Backup Display Configuration</h3>
      <div class="inline-centered">
        <md-button class="md-accent md-raised" @click="downloadBackup" :disabled="restoring">Download Backup</md-button>
        <md-tooltip md-delay="1000" md-direction="bottom"> Download a display configuration backup </md-tooltip>
      </div>
      <h3 style="margin-top: 30px">Restore Display Configuration</h3>
      <md-field>
        <label>Backup File</label>
        <md-file accept=".zip" @md-change="uploadFile" :disabled="restoring"/>
        <md-tooltip md-delay="1000" md-direction="bottom"> Backup file to restore from </md-tooltip>
      </md-field>
      <div class="inline-centered">
        <md-button class="md-accent md-raised" @click="upload" :disabled="restoring">Restore Backup</md-button>
        <md-tooltip md-delay="1000" md-direction="bottom"> Restore display configuration from backup file </md-tooltip>
      </div>
    </md-card>
  </div>
</template>

<script>
export default {
  name: "Backup",
  data: () => ({
    file: null,
    restoring: false
  }),
  methods: {
    uploadFile(files) {
      if (files.length === 0)
        return
      this.file = files[0]
    },
    async upload() {
      if (!this.file)
        return
      this.restoring = true
      await this.loadBackup(await this.file.arrayBuffer())
      this.restoring = false
    }
  }
}
</script>

<style scoped>
.md-card {
  width: 320px;
  display: inline-block;
}
</style>
