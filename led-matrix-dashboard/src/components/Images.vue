<template>
  <div>
    <div class="md-layout md-alignment-space-between-left">
      <md-button class="md-accent md-raised md-layout-item" @click="refreshImages(true)">Refresh</md-button>
      <md-button class="md-accent md-raised md-layout-item" @click="">Add Image</md-button>
    </div>
    <div>
      <md-card v-for="(data, image) in this.$store.state.imageData">
        <md-card-header class="md-title">{{ image | remove_P }}</md-card-header>
        <md-card-media>
          <image-preview :image="image"></image-preview>
        </md-card-media>
        <md-card-actions md-alignment="space-between">
          <md-button class="md-accent md-raised" @click="getImage(image)">Reload</md-button>
          <md-button class="md-accent md-raised" :href="'#/edit_image/' + image">Edit</md-button>
          <md-button :disabled="image.endsWith('_P')" class="md-accent md-raised" @click="deleteImageClick(image)">Delete</md-button>
        </md-card-actions>
      </md-card>
    </div>


    <md-dialog-confirm
        :md-active.sync="deleteConfirm"
        md-title="Delete Image"
        :md-content="'Are you sure you want to delete ' + deleteImageName + '?'"
        md-confirm-text="Delete"
        md-cancel-text="Cancel"
        @md-confirm="deleteImage" />
  </div>
</template>

<script>
import ImagePreview from "@/components/ImagePreview";
export default {
  name: "Images",
  components: {ImagePreview},
  async mounted() {
    await this.waitForPromiseSuccess(this.getStats, 2000)
    await this.refreshImages(Object.keys(this.$store.state.images).length === 0)
  },
  data: () => ({
    deleteConfirm: false,
    deleteImageName: ''
  }),
  methods: {
    deleteImageClick(image) {
      this.deleteImageName = image
      this.deleteConfirm = true
    },
    deleteImage() {
      this.$axios.post('/deleteImage', '', {params: {name: this.deleteImageName}}).then(response => {
        this.getImageData()
      }).catch(error => {
        console.log(error)
      })
    }
  }
}
</script>

<style scoped>
.md-card {
  width: 300px;
  margin: 4px;
  display: inline-block;
  vertical-align: top;
}
</style>
