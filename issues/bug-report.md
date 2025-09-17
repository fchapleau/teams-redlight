## Bug Report

### Issue
The GitHub Actions workflow fails due to the following reasons:
1. Missing 'firmware-build' artifact.
2. Missing `build/web/index.html`.
3. Permission denied to `github-actions[bot]` for pushing to `gh-pages`.

### Request for Fix
1. Verify artifact upload and naming.
2. Ensure `build/web/index.html` is generated before deployment.
3. Grant workflow permissions to allow GitHub Actions to push to `gh-pages`.

### Steps to Reproduce
1. Trigger the GitHub Actions workflow.
2. Observe the failure messages in the Actions tab.

### Expected Behavior
The workflow should complete successfully without errors regarding artifacts or permissions.