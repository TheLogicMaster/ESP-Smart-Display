<template>
  <div>
    <md-card>
      <md-card-header>
        <h1>
          Backup and Restore
        </h1>
      </md-card-header>
      <h3>Backup Display Configuration</h3>
      <md-button class="md-accent md-raised" @click="downloadBackup" :disabled="restoring">Download Backup</md-button>
      <h3 style="margin-top: 30px">Restore Display Configuration</h3>
      <md-field>
        <label>Backup File</label>
        <md-file accept=".zip" @md-change="uploadFile" :disabled="restoring"/>
      </md-field>
      <md-button class="md-accent md-raised" @click="upload" :disabled="restoring">Restore Backup</md-button>
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
